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
    // Get file pages/account/settings.html
    std::ifstream file("pages/account/settings.html");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t pos = content.find("{displayname}");
    if (pos != std::string::npos) {
        content.replace(pos, 13, user.DisplayName);
    }

    // replace {bio} with the user's bio
    pos = content.find("{bio}");
    if (pos != std::string::npos) {
        content.replace(pos, 5, user.Bio);
    }

    // replace {notifications} with the user's notification settings
    pos = content.find("{notifications}");
    if (pos != std::string::npos) {
        content.replace(pos, 15, user.GeneralNotifs ? "checked" : "");
    }
    
    // replace {status} with the user's status settings
    pos = content.find("{status}");
    if (pos != std::string::npos) {
        content.replace(pos, 8, user.ShowStatus ? "checked" : "");
    }

    // send
    res.setHeader("Content-Type", "text/html");
    res.send(content);
}

void AccountSecurity(const Link::Request& req, Link::Response& res) {
    User user = GetUser(req.getCookie("username"), req.getCookie("key"));
    if (!user.LoggedIn) {
        res.redirect("/login");
        return;
    }
    std::ifstream file("pages/account/security.html");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // replace {pin} with the user's pin settings
    size_t pos = content.find("{pinstatus}");
    if (pos != std::string::npos) {
        content.replace(pos, 11, user.PinStatus ? "checked" : "");
    }

    // replace {pin} with the user's pin settings
    pos = content.find("{pin}");
    if (pos != std::string::npos) {
        content.replace(pos, 5, user.Pin);
    }

    pos = content.find("{time}");
    if (pos != std::string::npos) {
        content.replace(pos, 6, user.PinTime);
    }

    // replace {notifs} with the notifs
    pos = content.find("{notifs}");
    if (pos != std::string::npos) {
        content.replace(pos, 8, user.LoginNotifs ? "checked" : "");
    }

    // send
    res.setHeader("Content-Type", "text/html");
    res.send(content);
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
    std::string logs = user.Logs;
    if (logs.length() >= 2 && logs.front() == '"' && logs.back() == '"') {
        logs = logs.substr(1, logs.length() - 2);
    }
    logs = KyberHelper::decrypt(logs, user.Key);
    size_t pos = content.find("{data}");
    if (pos != std::string::npos) {
        content.replace(pos, 6, logs);
    }

    // replace {save} with the user's save history settings
    pos = content.find("{save}");
    if (pos != std::string::npos) {
        content.replace(pos, 6, user.SaveHistory ? "checked" : "");
    }

    // replace {pin} with the user's pin settings
    pos = content.find("{retention}");
    if (pos != std::string::npos) {
        content.replace(pos, 11, user.DataRetention);
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
        AddLog("Password Changed", req.getIP(), user);
        DB("UPDATE Users SET Password = \""+encrypted+"\" WHERE Username = \""+user.Username+"\"");
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

void APIAccountSettings(const Link::Request& req, Link::Response& res) {
    try {
        User user = GetUser(req.getCookie("username"), req.getCookie("key"));
        std::string body = req.getBody();
        if (body.empty()) {
            res.status(400);
            res.json("{\"error\":\"Empty request body\"}");
            return;
        }
        // parse json
        auto json = n11::JsonValue::parse(body);
        std::string displayName = json["displayName"].stringify();
        if (displayName.length() >= 2 && displayName.front() == '"' && displayName.back() == '"') {
            displayName = displayName.substr(1, displayName.length() - 2);
        }
        std::string bio = json["bio"].stringify();
        if (bio.length() >= 2 && bio.front() == '"' && bio.back() == '"') {
            bio = bio.substr(1, bio.length() - 2);
        }
        bool notifs = json["emailNotify"].stringify() == "true";
        bool status = json["onlineStatus"].stringify() == "true";

        // if null set to previous
        if (displayName == "null") displayName = user.DisplayName;
        if (bio == "null") bio = user.Bio;
        if (json["emailNotify"].stringify() == "null") notifs = user.GeneralNotifs;
        if (json["onlineStatus"].stringify() == "null") status = user.ShowStatus;

        // update user
        user.DisplayName = displayName;
        user.Bio = bio;
        user.GeneralNotifs = notifs;
        user.ShowStatus = status;
        // send to DB with updated logs
        DB("UPDATE Users SET DisplayName = \""+displayName+"\", Bio = \""+bio+"\", GeneralNotifs = "+(notifs?"true":"false")+", ShowStatus = "+(status?"true":"false")+" WHERE Username = \""+user.Username+"\"");
        AddLog("Account Settings Updated", req.getIP(), user);
        res.json("{\"success\":true}");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        res.status(400);
        res.json("{\"error\":\"Invalid JSON format\"}");
    }
}


void APIAccountPrivacy(const Link::Request& req, Link::Response& res) {
    try {
        User user = GetUser(req.getCookie("username"), req.getCookie("key"));
        std::string body = req.getBody();
        if (body.empty()) {
            res.status(400);
            res.json("{\"error\":\"Empty request body\"}");
            return;
        }
        // parse json
        auto json = n11::JsonValue::parse(body);
        bool saveHistory = json["saveActivity"].stringify() == "true";
        std::string retention = json["retentionPeriod"].stringify();
        if (retention.length() >= 2 && retention.front() == '"' && retention.back() == '"') {
            retention = retention.substr(1, retention.length() - 2);
        }

        // if null set to previous
        if (json["saveActivity"].stringify() == "null") saveHistory = user.SaveHistory;
        if (json["retentionPeriod"].stringify() == "null") retention = user.DataRetention;

        std::cout << "SaveHistory: " << saveHistory << std::endl;
        std::cout << "Retention: " << retention << std::endl;

        // update user
        user.SaveHistory = saveHistory;
        user.DataRetention = retention;

        // Fix: Use proper string concatenation for the SQL query
        std::string query = "UPDATE Users SET SaveHistory = ";
        query += (saveHistory ? "true" : "false");
        query += ", DataRetention = \"";
        query += retention;
        query += "\" WHERE Username = \"";
        query += user.Username;
        query += "\"";

        // send to DB with updated logs
        DB(query);
        AddLog("Privacy Settings Updated", req.getIP(), user);
        res.json("{\"success\":true}");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        res.status(400);
        res.json("{\"error\":\"Invalid JSON format\"}");
    }
}

void APIAccountSecurity(const Link::Request& req, Link::Response& res) {
    try {
        User user = GetUser(req.getCookie("username"), req.getCookie("key"));
        std::string body = req.getBody();
        if (body.empty()) {
            res.status(400);
            res.json("{\"error\":\"Empty request body\"}");
            return;
        }
        // parse json {pinEnabled: true, pinTimeout: "5", loginNotifications: false, pinCode: "3243"}
        auto json = n11::JsonValue::parse(body);
        bool pinStatus = json["pinEnabled"].stringify() == "true";
        std::string pin = json["pinCode"].stringify();
        if (pin.length() >= 2 && pin.front() == '"' && pin.back() == '"') {
            pin = pin.substr(1, pin.length() - 2);
        }
        bool loginNotifs = json["loginNotifications"].stringify() == "true";
        std::string pinTime = json["pinTimeout"].stringify();
        if (pinTime.length() >= 2 && pinTime.front() == '"' && pinTime.back() == '"') {
            pinTime = pinTime.substr(1, pinTime.length() - 2);
        }

        // if null set to previous
        if (json["pinEnabled"].stringify() == "null") pinStatus = user.PinStatus;
        if (json["pinCode"].stringify() == "null") pin = user.Pin;
        if (json["loginNotifications"].stringify() == "null") loginNotifs = user.LoginNotifs;
        if (json["pinTimeout"].stringify() == "null") pinTime = user.PinTime;

        // update user
        user.PinStatus = pinStatus;
        user.Pin = pin;
        user.LoginNotifs = loginNotifs;
        user.PinTime = pinTime;

        // build DB query
        std::string query = "UPDATE Users SET PinStatus = ";
        query += (pinStatus ? "true" : "false");
        query += ", Pin = \"";
        query += pin;
        query += "\", LoginNotifs = ";
        query += (loginNotifs ? "true" : "false");
        query += ", PinTime = \"";
        query += pinTime;
        query += "\" WHERE Username = \"";
        query += user.Username;
        query += "\"";

        // send to DB with updated logs
        DB(query);
        AddLog("Security Settings Updated", req.getIP(), user);
        res.json("{\"success\":true}");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        res.status(400);
        res.json("{\"error\":\"Invalid JSON format\"}");
    }
}