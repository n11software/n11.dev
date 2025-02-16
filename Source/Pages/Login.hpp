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

void APISignup(const Link::Request& req, Link::Response& res) {
    try {
        // Check if user exists
        User u = GetUser(req.getCookie("username"));
        if (u.Username == req.getCookie("username")) {
            res.json("{\"error\":\"User already exists\"}");
            return;
        }
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
        user["Created"].setRaw("time::now()");
        user["ProfilePicture"].setRaw("NULL");
        std::string logs = "[{\"time\":\""+getCurrentTimeISO8601()+"\",\"action\":\"Account Created\", \"ip\":\""+req.getIP()+"\"}]";
        user["Logs"] = KyberHelper::encrypt(logs, keyPair.publicKey).ciphertext;
        std::string end = user.stringify();

        DB("INSERT INTO Users "+end).stringify();
        res.json("{\"success\":\"true\"}");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        res.status(400);
        res.json("{\"error\":\"Invalid JSON format\"}");
    }
}

void APIProfile(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getParam("user"));
    if (user.Username.empty()) {
        res.json("{\"error\":\"User doesn't exist\"}");
        return;
    }
    n11::JsonValue json;
    json["username"] = user.Username;
    json["created"] = user.CreatedAt;
    json["pfp"] = user.PFP;
    res.json(json.stringify());
}

void APILogin(const Link::Request& req, Link::Response& res) {
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
        User user = GetUser(username, password);
        if (user.LoggedIn) {
            // Get logs
            AddLog("Logged In", req.getIP(), user);
            res.json("{\"success\": true}");
        } else {
            res.json("{\"error\":\"Failed to authenticate\"}");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        res.json("{\"error\":\"Unknown error\"}");
    }
}