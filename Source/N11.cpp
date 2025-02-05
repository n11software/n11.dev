#include "Json.hpp"
#include "Link.hpp"
#include "KyberHelper.hpp"
#include "AES256.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include "ImageToPNG.hpp"

struct User {
    std::string CreatedAt, Cert, Key, PFP, Username;
    bool LoggedIn;
};

n11::JsonValue DB(std::string query) {
    try {
        // Create record
        Link::Client client(true);  // Enable SSL
        client.setVerifyPeer(false);
        std::vector<unsigned char> auth(9);  // "root:root" is 9 chars including null terminator
        std::copy_n("root:root", 9, auth.begin());
        client.setHeader("Authorization", "Basic " + AES256::base64_encode(auth));
        client.setHeader("Accept", "application/json");
        client.setHeader("surreal-ns", "Development");
        client.setHeader("surreal-db", "N11");
        auto response = client.Post("http://localhost:8000/sql", query);
        if (response.getStatusCode() != 200) {
            return n11::JsonValue().parse("{\"error\":\"Failed to fetch data\"}");
        }
        auto data = n11::JsonValue::parse(response.getBody());
        return data;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return n11::JsonValue().parse("{\"error\":\"Failed to fetch data\"}");
    }
}

User GetUser(std::string username, std::string key) {
    auto data = DB("SELECT * FROM Users WHERE Username = \""+username+"\"");
    User user;
    try {
        if (data.hasKey("error") && data["error"].stringify() != "null") {
            user.LoggedIn = false;
            return user;
        }

        // The result array is in data[0]["result"]
        if (!data[0].hasKey("result") || !data[0]["result"].isArray() || 
            data[0]["result"].getArray().empty()) {
            user.LoggedIn = false;
            return user;
        }

        auto& userObj = data[0]["result"][0];
        
        // Extract username safely
        std::string username = "";
        if (userObj.hasKey("Username")) {
            username = userObj["Username"].stringify();
            if (username.length() >= 2 && username.front() == '"' && username.back() == '"') {
                username = username.substr(1, username.length() - 2);
            }
        }

        // Extract date creation time
        std::string created = "";
        if (userObj.hasKey("Password")) {
            created = userObj["Password"].stringify();
            if (created.length() >= 2 && created.front() == '"' && created.back() == '"') {
                created = created.substr(1, created.length() - 2);
            }
        }

        std::string password = "";
        if (userObj.hasKey("Password")) {
            password = userObj["Password"].stringify();
            if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
                password = password.substr(1, password.length() - 2);
            }
        }

        // Verify account ownership
        std::string pw = AES256::decrypt(password, key);
        // Validate decrypted JSON
        try {
            if (pw.empty()) {
                user.LoggedIn = false;
                return user;
            }
            auto keyPair = n11::JsonValue::parse(pw);
            if (!keyPair.hasKey("publicKey") || !keyPair.hasKey("privateKey")) {
                user.LoggedIn = false;
                return user;
            }
            user.Cert = keyPair["publicKey"].stringify();
            user.Key = keyPair["privateKey"].stringify();
            user.LoggedIn = true;
        } catch (const std::exception& e) {
            user.LoggedIn = false;
            return user;
        }

        // Extract profile picture safely
        std::string pfp = "{ \"error\": \"Not logged in\" }";
        if (userObj.hasKey("ProfilePicture")) {
            pfp = userObj["ProfilePicture"].stringify();
            if (pfp.length() >= 2 && pfp.front() == '"' && pfp.back() == '"') {
                pfp = pfp.substr(1, pfp.length() - 2);
            }
            // create a new json object with the profile picture
            n11::JsonValue pfpJson;
            pfpJson["pfp"] = pfp;
            std::string json = pfpJson.stringify();
            if (json.find("\"null\"") != std::string::npos) {
                pfp = json.replace(json.find("\"null\""), 6, "null");
            }
            pfp = json;
            // replace "null" with null
        }
        user.Username = username;
        user.PFP = pfp;
        user.CreatedAt = created;
        return user;
    } catch (...) {
        user.LoggedIn = false;
        return user;
    }
}

int main() {
    try {
        Link::Server server(true);  // Enable metrics
        
        server.Get("/", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/index.html");
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
        
        server.Get("/account/profile", [](const Link::Request& req, Link::Response& res) {
            res.sendFile("pages/account/profile.html");
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
            res.sendFile("cache/" + filename);
        });

        server.Post("/api/user/updatepfp", [](const Link::Request& req, Link::Response& res) {
            User user = GetUser(req.getCookie("username"), req.getCookie("key"));
            std::string body = req.getBody();
            if (body.empty()) {
                std::cout << "Empty body received" << std::endl;
                res.status(400);
                res.json("{\"error\":\"Empty request body\"}");
                return;
            }

            // Convert body to vector for decoding
            auto decoded = std::string(body.begin(), body.end());
            std::string pngOutputData;

            try {
                if (convertToPNG(decoded, pngOutputData)) {
                    // write to file
                    std::ofstream file("test.png");
                    file << pngOutputData;
                    file.close();
                    std::vector<unsigned char> pngDataVec(pngOutputData.begin(), pngOutputData.end());
                    std::string encoded = AES256::base64_encode(pngDataVec);
                    // send to DB
                    DB("UPDATE Users SET ProfilePicture = \""+encoded+"\" WHERE Username = \""+user.Username+"\"");
                } else {
                    std::cerr << "Failed to convert image." << std::endl;
                    res.json("\"error\":\"Filetype not supported\"");
                }
            } catch (...) {
                std::cerr << "Failed to convert image." << std::endl;
                res.json("\"error\":\"Filetype not supported\"");
            }
            
            res.json("{\"success\":true}");
        });

        server.Post("/api/login", [](const Link::Request& req, Link::Response& res) {
            try {
                std::string rawBody = req.getBody();
                size_t jsonStart = rawBody.find("\r\n\r\n");
                if (jsonStart == std::string::npos) {
                    jsonStart = rawBody.find("\n\n");
                }
                if (jsonStart != std::string::npos) {
                    rawBody = rawBody.substr(jsonStart + 4);
                }
                
                auto json = n11::JsonValue::parse(rawBody);
                auto password = json["password"].stringify();
                if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
                    password = password.substr(1, password.length() - 2);
                }
                std::string username = json["username"].stringify();
                if (username.length() >= 2 && username.front() == '"' && username.back() == '"') {
                    username = username.substr(1, username.length() - 2);
                }
                std::cout << "Username: " << username << "\nPassword: " << password << std::endl;
                User user = GetUser(username, password);
                if (user.LoggedIn) {
                    res.json("{\"success\": true}");
                } else {
                    res.json("{\"error\":\"Failed to authenticate\"}");
                }
            } catch (...) {
                res.json("{\"error\":\"Unknown error\"}");
            }
        });

        server.Post("/api/signup", [](const Link::Request& req, Link::Response& res) {
            try {
                std::string rawBody = req.getBody();
                size_t jsonStart = rawBody.find("\r\n\r\n");
                if (jsonStart == std::string::npos) {
                    jsonStart = rawBody.find("\n\n");
                }
                if (jsonStart != std::string::npos) {
                    rawBody = rawBody.substr(jsonStart + 4);
                }
                
                auto json = n11::JsonValue::parse(rawBody);
                auto password = json["password"].stringify();
                if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
                    password = password.substr(1, password.length() - 2);
                }
                
                // Generate a new key pair
                auto keyPair = KyberHelper::generateKeyPair();
                if (!keyPair.success()) {
                    res.json("{\"error\":\"" + keyPair.error + "\"}");
                    return;
                }
                bool isValid = KyberHelper::verifyKeyPair(keyPair.publicKey, keyPair.privateKey);
                if (!isValid) {
                    res.json("{\"error\":\"Invalid key pair\"}");
                    return;
                }

                // Create a json with the public key and private key
                n11::JsonValue jsonKeyPair;
                jsonKeyPair["publicKey"] = keyPair.publicKey;
                jsonKeyPair["privateKey"] = keyPair.privateKey;

                // Encrypt the json with AES256
                auto encrypted = AES256::encrypt(jsonKeyPair.stringify(), password);
                if (encrypted.empty()) {
                    res.json("{\"error\":\"AES256 encryption failed\"}");
                    return;
                }

                n11::JsonValue user;
                std::string username = json["username"].stringify();
                if (username.length() >= 2 && username.front() == '"' && username.back() == '"') {
                    username = username.substr(1, username.length() - 2);
                }
                if (username.find("time::now()") != std::string::npos && username.find("NULL") != std::string::npos) {
                    res.json("{\"error\":\"Invalid username\"}");
                    return;
                }
                user["Username"] = username;
                user["Password"] = encrypted;
                user["Created"] = "time::now()";
                user["ProfilePicture"] = "NULL";

                std::string end = user.stringify();
                // replace the "\"time::now()\"" with "time::now()"
                end = end.replace(end.find("\"time::now()\""), 13, "time::now()");
                // replace the "\"NULL\"" with "NULL"
                end = end.replace(end.find("\"NULL\""), 6, "NULL");

                DB("INSERT INTO Users "+end).stringify();
                res.json("{\"success\":\"true\"}");
            } catch (const std::exception& e) {
                res.status(400);
                res.json("{\"error\":\"Invalid JSON format\"}");
            }
        });

        server.Get("/enctest", [](const Link::Request& req, Link::Response& res) {
            n11::JsonValue json;
            
            auto keyPair = KyberHelper::generateKeyPair();
            if (!keyPair.success()) {
                json["error"] = keyPair.error;
                res.json(json.stringify());
                return;
            }

            bool isValid = KyberHelper::verifyKeyPair(keyPair.publicKey, keyPair.privateKey);
            
            auto msgPair = KyberHelper::encrypt("Hello, World!", keyPair.publicKey);
            if (!msgPair.success()) {
                json["error"] = msgPair.error;
                res.json(json.stringify());
                return;
            }

            auto decryptedMessage = KyberHelper::decrypt(
                msgPair.ciphertext,
                msgPair.sharedSecret,
                keyPair.privateKey
            );


            json["publicKey"] = keyPair.publicKey;
            json["privateKey"] = keyPair.privateKey;
            auto encrypted = AES256::encrypt(json.stringify(), "password");
            auto decrypted = AES256::decrypt(encrypted, "password");
            if (json.stringify() != decrypted) {
                json["error"] = "AES256 encryption/decryption failed";
                res.json(json.stringify());
                return;
            }
            json["AES256"] = "Working!";
            json["verified"] = isValid;
            json["kyberCiphertext"] = msgPair.ciphertext;
            json["sharedSecret"] = msgPair.sharedSecret;
            json["message"] = decryptedMessage;
            
            res.json(json.stringify());
        });

        // Custom 404 handler
        server.OnError(404, [](const Link::Request& req, Link::Response& res, int code) {
            // check if file exists in /public
            std::string path = "public" + req.getURL().getPath();
            if (std::filesystem::exists(path)) {
                res.sendFile(path);
                return;
            }
            res.status(code);
            res.send(req.getMethod()+" "+req.getURL().getPath());
        });

        // Default error handler for all other errors
        server.OnError([](const Link::Request& req, Link::Response& res, int code) {
            res.status(code);
            res.send("Oops! Something went wrong: " + std::to_string(code));
        });

        std::cout << "Server starting on port 3000..." << std::endl;
        server.Listen(3000);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}