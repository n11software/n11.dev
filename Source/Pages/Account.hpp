#pragma once
#include <string>
#include "../Json.hpp"
#include <Link.hpp>
#include "../KyberHelper.hpp"
#include "../AES256.hpp"
#include <iostream>
#include "../DB.hpp"
#include <chrono>
#include "../Users.hpp"
#include <fstream>

void Accounts(const Link::Request& req, Link::Response& res) {
    // Read json from cookie "offload"
    std::string offloadStr = req.getCookie("offload");
    if (offloadStr.empty()) {
        res.redirect("/login");
        return;
    }
    n11::JsonValue offload = n11::JsonValue::parse(offloadStr);
    if (offload.hasKey("error")) {
        res.json(offload.stringify());
        return;
    }
    std::map<std::string, std::string> accounts;
    for (auto& [key, value] : offload.getObject()) {
        std::string password = value.stringify();
        if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
            password = password.substr(1, password.length() - 2);
        }
        accounts[key] = password;
    }

    std::ifstream file("pages/accounts.html");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string rep;
    for (auto& [key, value] : accounts) {
        User user = GetUser(key, value);
        if (!user.LoggedIn) continue;
        // parse pfp json
        n11::JsonValue pfpJson = n11::JsonValue::parse(user.PFP);
        std::string pfp = pfpJson["pfp"].stringify();
        if (pfp.length() >= 2 && pfp.front() == '"' && pfp.back() == '"') {
            pfp = pfp.substr(1, pfp.length() - 2);
        } else {
            pfp = "null";
        }
        
        rep += "<div class='account'> \
            <img src='"+(pfp=="null"?"/Default.jpg":"data:image/png;base64,"+pfp)+"' alt='Avatar' class='avatar'/>\
            <div>\
                <span class='name'>"+user.Username+"</span>\
                <span class='email'>"+user.Username+"@n11.dev</span>\
            </div>\
        </div>";
    }
    // replace {accounts} with the generated accounts
    size_t pos = content.find("{accounts}");
    if (pos != std::string::npos) {
        content.replace(pos, 10, rep);
    }

    res.setHeader("Content-Type", "text/html");
    res.send(content);
}

void Account(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    res.redirect("/account/settings");
}

void AccountSettings(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    res.sendFile("pages/account/settings.html");
}

void AccountPrivacy(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    // get pages/account/privacy.html
    std::ifstream file("pages/account/privacy.html");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // replace {data} with the user's logs
    size_t pos = content.find("{data}");
    if (pos != std::string::npos) {
        content.replace(pos, 6, user.Logs);
    }
    res.setHeader("Content-Type", "text/html");
    res.send(content);
}

void AccountPassword(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    res.sendFile("pages/account/password.html");
}

void APIPassword(const Link::Request& req, Link::Response& res) {
    std::string body = req.getBody();
    if (body.empty()) {
        res.status(400);
        res.json("{\"error\":\"Empty request body\"}");
        return;
    }
    // parse json
    auto json = n11::JsonValue::parse(body);
    std::string password = json["password"].stringify();
    if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.length() - 2);
    }
    std::cout << "Password: " << password << std::endl;
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (user.LoggedIn) {
        // Encrypt user.cert and user.key with the new password
        n11::JsonValue jsonKeyPair;
        jsonKeyPair["publicKey"].setRaw(user.Cert);
        jsonKeyPair["privateKey"].setRaw(user.Key);
        // Encrypt the json with AES256
        auto encrypted = AES256::encrypt(jsonKeyPair.stringify(), password);
        if (encrypted.empty()) {
            res.json("{\"error\":\"AES256 encryption failed\"}");
            return;
        }
        if (AES256::decrypt(encrypted, password) != jsonKeyPair.stringify()) {
            res.json("{\"error\":\"AES256 decryption failed\"}");
            return;
        }
        DB("UPDATE Users SET Password = \""+encrypted+"\" WHERE Username = \""+user.Username+"\"");
        AddLog("Password Changed", req.getIP(), user);
        res.json("{\"success\":true}");
    } else {
        res.json("{\"error\":\"Failed to authenticate\"}");
    }
}

void APIVerify(const Link::Request& req, Link::Response& res) {
    std::string body = req.getBody();
    if (body.empty()) {
        res.status(400);
        res.json("{\"error\":\"Empty request body\"}");
        return;
    }
    // parse json
    auto json = n11::JsonValue::parse(body);
    std::string password = json["password"].stringify();
    if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.length() - 2);
    }
    User user = GetUser(req.getCookie("username"), password);
    if (user.LoggedIn) {
        res.json("{\"success\": true}");
    } else {
        res.json("{\"error\":\"Failed to authenticate\"}");
    }
}

void AccountDelete(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    res.sendFile("pages/account/delete.html");
}

void APIDelete(const Link::Request& req, Link::Response& res) {
    std::string body = req.getBody();
    if (body.empty()) {
        res.status(400);
        res.json("{\"error\":\"Empty request body\"}");
        return;
    }
    // parse json
    auto json = n11::JsonValue::parse(body);
    std::string password = json["password"].stringify();
    if (password.length() >= 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.length() - 2);
    }
    User user = GetUser(req.getCookie("username"), password);
    if (user.LoggedIn) {
        DB("DELETE Users WHERE Username = \""+user.Username+"\"");
        res.json("{\"success\": true}");
    } else {
        res.json("{\"error\":\"Failed to authenticate\"}");
    }
}

void APIPFP(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    std::string body = req.getBody();
    if (body.empty()) {
        res.status(400);
        res.json("{\"error\":\"Empty request body\"}");
        return;
    }

    // Convert body to vector for decoding
    auto decoded = std::string(body.begin(), body.end());
    std::string pngOutputData;

    try {
        if (convertToPNG(decoded, pngOutputData)) {
            std::vector<unsigned char> pngDataVec(pngOutputData.begin(), pngOutputData.end());
            std::string encoded = AES256::base64_encode(pngDataVec);
            
            AddLog("Profile Picture Updated", req.getIP(), user);

            // send to DB with updated logs
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
}