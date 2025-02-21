#pragma once
#include <string>
#include "Json.hpp"
#include <Link.hpp>
#include "KyberHelper.hpp"
#include "AES256.hpp"
#include <iostream>

n11::JsonValue DB(std::string query) {
    try {
        // start high performance timer
        // Create record
        Link::Client client(false);  // Enable SSL
        // client.setVerifyPeer(false);
        std::vector<unsigned char> auth(9);  // "root:root" is 9 chars including null terminator
        std::copy_n("root:root", 9, auth.begin());
        client.setHeader("Authorization", "Basic " + AES256::base64_encode(auth));

        client.setHeader("Accept", "application/json");
        client.setHeader("surreal-ns", "N11");
        client.setHeader("surreal-db", "N11");
        client.setHeader("accept-encoding", "gzip");
        client.enableMetrics(true);
        auto start = std::chrono::high_resolution_clock::now();
        auto response = client.Post("http://localhost:8000/sql", query);
        if (response.getStatusCode() != 200) {
            return n11::JsonValue().parse("{\"error\":\"Failed to fetch data\"}");
        }
        auto data = n11::JsonValue::parse(response.getBody());
        auto end = std::chrono::high_resolution_clock::now();
        // stop high performance timer
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "DB query took " << elapsed.count() << " seconds" << std::endl;
        return data;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return n11::JsonValue().parse("{\"error\":\"Failed to fetch data\"}");
    }
}