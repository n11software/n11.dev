#pragma once
#include <string>
#include "Json.hpp"
#include <Link.hpp>
#include "KyberHelper.hpp"
#include "AES256.hpp"
#include <iostream>
#include "DB.hpp"
#include <chrono>

std::string getCurrentTimeISO8601() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';
    return ss.str();
}

struct User {
    std::string CreatedAt = "", Cert = "", Key = "", PFP = "", Username = "", Logs = "";
    bool LoggedIn = false;
};

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
        if (userObj.hasKey("Created")) {
            created = userObj["Created"].stringify();
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
        user.Logs = userObj["Logs"].stringify();
        return user;
    } catch (...) {
        user.LoggedIn = false;
        return user;
    }
}

User GetUser(std::string username) {
    auto data = DB("SELECT * FROM Users WHERE Username = \""+username+"\"");
    User user;
    try {
        if (data.hasKey("error") && data["error"].stringify() != "null") {
            return user;
        }

        // The result array is in data[0]["result"]
        if (!data[0].hasKey("result") || !data[0]["result"].isArray() || 
            data[0]["result"].getArray().empty()) {
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
        if (userObj.hasKey("Created")) {
            created = userObj["Created"].stringify();
            if (created.length() >= 2 && created.front() == '"' && created.back() == '"') {
                created = created.substr(1, created.length() - 2);
            }
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
        return user;
    }
}

void AddLog(std::string val, std::string ip, User user) {
    auto data = DB("SELECT Logs FROM Users WHERE Username = \""+user.Username+"\"");
    std::string logs = data[0]["result"][0]["Logs"].stringify();
    // parse
    n11::JsonValue logsJson = n11::JsonValue::parse(logs);
    // Add new log
    auto log = n11::JsonValue();
    // get time in this format yyyy-MM-ddThh:mm:ss.msZ
    std::string time = getCurrentTimeISO8601();
    log["time"] = time;
    log["action"] = val;
    log["ip"] = ip;
    logsJson.push_back(log);
    DB("UPDATE Users SET Logs = '"+logsJson.stringify()+"' WHERE Username = \""+user.Username+"\"");
}