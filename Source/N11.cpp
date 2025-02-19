#include "Json.hpp"
#include <Link.hpp>
#include "KyberHelper.hpp"
#include "AES256.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include "ImageToPNG.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include "Users.hpp"
#include "DB.hpp"
#include "Pages/Account.hpp"
#include "Pages/Login.hpp"

int main() {
    // read config file
    std::ifstream file("config.json");
    std::string config((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    n11::JsonValue configJson = n11::JsonValue::parse(config);
    if (!configJson.hasKey("port")) {
        std::cerr << "Failed to read config file" << std::endl;
        return 1;
    }
    if (configJson["port"].stringify() == "null") {
        std::cerr << "Failed to read port from config file" << std::endl;
        return 1;
    }
    int port = std::stoi(configJson["port"].stringify());
    if (!configJson.hasKey("cert")) {
        std::cerr << "Failed to read config file" << std::endl;
        return 1;
    }
    if (configJson["cert"].stringify() == "null") {
        std::cerr << "Failed to read cert from config file" << std::endl;
        return 1;
    }
    std::string cert = configJson["cert"].stringify();
    if (!configJson.hasKey("key")) {
        std::cerr << "Failed to read config file" << std::endl;
        return 1;
    }
    if (configJson["cert"].stringify() == "null") {
        std::cerr << "Failed to read cert from config file" << std::endl;
        return 1;
    }
    if (configJson["key"].stringify() == "null") {
        std::cerr << "Failed to read key from config file" << std::endl;
        return 1;
    }
    std::string key = configJson["key"].stringify();
    if (cert.length() >= 2 && cert.front() == '"' && cert.back() == '"') {
        cert = cert.substr(1, cert.length() - 2);
    }
    if (key.length() >= 2 && key.front() == '"' && key.back() == '"') {
        key = key.substr(1, key.length() - 2);
    }

    try {
        Link::Server server(true, cert, key);  // Enable metrics
        
        server.Get("/", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/index.html");
        });

        server.Get("/gztest", [](const Link::Request& req, Link::Response& res) {
            std::string body = "Hello, World! This is a test of gzip compression.";
            
            res.send(body);
        });
        
        server.Get("/accounts", Accounts);
        server.Get("/account", Account);
        server.Get("/profile/[user]", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/profile.html");
        });
        server.Get("/account/settings", AccountSettings);
        server.Get("/account/privacy", AccountPrivacy);
        server.Get("/account/password", AccountPassword);
        server.Get("/account/security", AccountSecurity);
        server.Post("/api/account/password", APIPassword);
        server.Post("/api/account/settings/update", APIAccountSettings);
        server.Post("/api/account/security/update", APIAccountSecurity);
        server.Post("/api/account/privacy/update", APIAccountPrivacy);
        server.Post("/api/account/verify", APIVerify);
        server.Get("/account/delete", AccountDelete);
        server.Post("/api/account/delete", APIDelete);
        server.Post("/api/user/updatepfp", APIPFP);
        server.Post("/api/login", APILogin);
        server.Get("/api/user/profile/[user]", APIProfile);
        server.Post("/api/signup", APISignup);

        server.Get("/enctest", [](const Link::Request& req, Link::Response& res) {
            // Get user
            User user = GetUser(req.getCookie("username"), req.getCookie("key"));
            if (!user.LoggedIn) {
                res.redirect("/login");
                return;
            }
            // Encrypt message
            std::string message = user.Logs;
            if (message.length() >= 2 && message.front() == '"' && message.back() == '"') {
                message = message.substr(1, message.length() - 2);
            }
            // decrypt message
            std::string dec = KyberHelper::decrypt(message, user.Key);
            if (dec.empty()) {
                res.json("{\"error\":\"Decryption failed\"}");
                return;
            }
            n11::JsonValue json;
            json["decrypted"].setRaw(dec);
            res.json(json.stringify());
        });
        
        server.Get("/login", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/login.html");
        });
        
        server.Get("/signup", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/signup.html");
        });
        
        server.Get("/link", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/link.html");
        });

        server.Get("/cache/[filename]", [](const Link::Request& req, Link::Response& res) {
            std::string filename = req.getParam("filename");
            res.setHeader("Cache-Control", "public, max-age=3600");
            if (filename == "navbar.js") {
                std::ifstream file("cache/navbar.js");
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                User user = GetUser(req.getCookie("username"), req.getCookie("key"));

                if (!user.LoggedIn) {
                    res.sendFile("cache/navbar.js");
                    return;
                }

                // Safe string replacements
                auto pos = content.find("{email}");
                if (pos != std::string::npos) {
                    content.replace(pos, 7, user.Username + "@n11.dev");
                }

                pos = content.find("'{pfp}'");
                if (pos != std::string::npos) {
                    content.replace(pos, 7, user.PFP);
                }

                pos = content.find("{username}");
                if (pos != std::string::npos) {
                    content.replace(pos, 10, user.Username);
                }

                res.setHeader("Content-Type", "application/javascript");
                res.send(content);
                return;
            }
            // check if file exists in /public
            std::string path = "cache/" + filename;
            if (path.find("..") != std::string::npos) {
                res.status(403);
                res.send("Forbidden");
                return;
            }
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                res.sendFile(path);
                return;
            }
            res.status(404);
            res.sendFile("pages/404.html");
        });

        // Custom 404 handler
        server.OnError(404, [](const Link::Request& req, Link::Response& res, int code) {
            // check if file exists in /public
            std::string path = "public" + req.getURL().getPath();
            if (path.find("..") != std::string::npos) {
                res.status(403);
                res.send("Forbidden");
                return;
            }
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                res.sendFile(path);
                return;
            }
            res.status(code);
            res.sendFile("pages/404.html");
        });

        // Default error handler for all other errors
        server.OnError([](const Link::Request& req, Link::Response& res, int code) {
            res.status(code);
            res.send("Oops! Something went wrong: " + std::to_string(code));
        });

        std::cout << "Server starting on port "<< port <<"..." << std::endl;
        server.Listen(port);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}