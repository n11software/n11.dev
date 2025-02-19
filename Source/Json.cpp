#include "Json.hpp"
#include <sstream>
#include <iomanip>
#include <string_view>

namespace n11 {

JsonValue JsonValue::parse(const std::string& json) {
    std::string_view view(json);
    size_t pos = 0;
    skipWhitespace(view, pos);
    return parseValue(view, pos);
}

JsonValue JsonValue::parseValue(std::string_view json, size_t& pos) {
    skipWhitespace(json, pos);
    
    if (pos >= json.length()) {
        throw std::runtime_error("Unexpected end of input");
    }

    switch (json[pos]) {
        case 'n':
            if (pos + 4 <= json.length() && json.substr(pos, 4) == "null") {
                pos += 4;
                return JsonValue(nullptr);
            }
            break;
        case 't':
            if (pos + 4 <= json.length() && json.substr(pos, 4) == "true") {
                pos += 4;
                return JsonValue(true);
            }
            break;
        case 'f':
            if (pos + 5 <= json.length() && json.substr(pos, 5) == "false") {
                pos += 5;
                return JsonValue(false);
            }
            break;
        case '"':
        case '\'':
            return JsonValue(parseString(json, pos));
        case '[':
            return JsonValue(parseArray(json, pos));
        case '{':
            return JsonValue(parseObject(json, pos));
        default:
            if (json[pos] == '-' || std::isdigit(json[pos])) {
                return JsonValue(parseNumber(json, pos));
            }
    }
    
    throw std::runtime_error("Invalid JSON syntax");
}

std::string JsonValue::stringify() const {
    std::ostringstream oss;
    
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            oss << "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            oss << (v ? "true" : "false");
        } else if constexpr (std::is_same_v<T, double>) {
            oss << std::fixed << std::setprecision(6) << v;
            std::string str = oss.str();
            if (str.find('.') != std::string::npos) {
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.back() == '.') str.pop_back();
            }
            oss.str("");
            oss << str;
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (v.length() > 4 && v.substr(0, 4) == "RAW:") {
                oss << v.substr(4);
            } else if (v.find('{') == 0 || v.find('[') == 0) {
                // Already JSON-formatted string
                oss << v;
            } else {
                // Handle special characters and escape quotes
                oss << '"';
                for (char c : v) {
                    switch (c) {
                        case '"': oss << "\\\""; break;
                        case '\'': oss << "'"; break;  // Don't escape single quotes
                        case '\\': oss << "\\\\"; break;
                        case '\b': oss << "\\b"; break;
                        case '\f': oss << "\\f"; break;
                        case '\n': oss << "\\n"; break;
                        case '\r': oss << "\\r"; break;
                        case '\t': oss << "\\t"; break;
                        default: oss << c;
                    }
                }
                oss << '"';
            }
        } else if constexpr (std::is_same_v<T, Array>) {
            oss << '[';
            bool first = true;
            for (const auto& elem : v) {
                if (!first) oss << ',';
                std::string elemStr = elem.stringify();
                if (elemStr.length() >= 2 && elemStr.front() == '"' && elemStr.back() == '"' 
                    && (elemStr.find('{') != std::string::npos || elemStr.find('[') != std::string::npos)) {
                    // Remove outer quotes for nested JSON
                    elemStr = elemStr.substr(1, elemStr.length() - 2);
                }
                oss << elemStr;
                first = false;
            }
            oss << ']';
        } else if constexpr (std::is_same_v<T, Object>) {
            oss << '{';
            bool first = true;
            for (const auto& [key, value] : v) {
                if (!first) oss << ',';
                oss << '"' << key << "\":";
                std::string valueStr = value.stringify();
                if (valueStr.length() >= 2 && valueStr.front() == '"' && valueStr.back() == '"'
                    && (valueStr.find('{') != std::string::npos || valueStr.find('[') != std::string::npos)) {
                    // Remove outer quotes for nested JSON
                    valueStr = valueStr.substr(1, valueStr.length() - 2);
                }
                oss << valueStr;
                first = false;
            }
            oss << '}';
        }
    }, value_);
    
    return oss.str();
}

// Helper parsing functions implementation would go here
// ...

std::string JsonValue::parseString(std::string_view json, size_t& pos) {
    char quoteChar = json[pos];
    if (quoteChar != '"' && quoteChar != '\'') {
        throw std::runtime_error("Expected quote at start of string");
    }
    
    std::string result;
    result.reserve(32); // Reserve some initial capacity
    pos++; // Skip opening quote
    
    size_t start = pos;
    while (pos < json.length()) {
        char c = json[pos];
        if (c == quoteChar) {
            if (pos > start) {
                result.append(json.data() + start, pos - start);
            }
            pos++;
            return result;
        }
        if (c == '\\' && pos + 1 < json.length()) {
            if (pos > start) {
                result.append(json.data() + start, pos - start);
            }
            pos++;
            char next = json[pos++];
            switch (next) {
                case '"':
                case '\'': result += next; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += next;
            }
            start = pos;
        } else {
            pos++;
        }
    }
    
    throw std::runtime_error("Unterminated string");
}

double JsonValue::parseNumber(std::string_view json, size_t& pos) {
    size_t start = pos;
    if (json[pos] == '-') pos++;
    
    while (pos < json.length() && std::isdigit(json[pos])) pos++;
    if (pos < json.length() && json[pos] == '.') {
        pos++;
        while (pos < json.length() && std::isdigit(json[pos])) pos++;
    }
    
    return std::stod(std::string(json.substr(start, pos - start)));
}

void JsonValue::skipWhitespace(std::string_view json, size_t& pos) {
    while (pos < json.length() && isWhitespace(json[pos])) {
        pos++;
    }
}

bool JsonValue::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

JsonValue::Array JsonValue::parseArray(std::string_view json, size_t& pos) {
    pos++; // Skip '['
    
    Array array;
    array.reserve(16); // Reserve space for 16 elements initially
    skipWhitespace(json, pos);
    
    while (pos < json.length()) {
        if (json[pos] == ']') {
            pos++;
            return array;
        }

        array.push_back(parseValue(json, pos));
        skipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unterminated array");
        }
        
        if (json[pos] == ']') {
            pos++;
            return array;
        }
        
        if (json[pos] != ',') {
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        
        pos++;
        skipWhitespace(json, pos);
    }
    
    throw std::runtime_error("Unterminated array");
}

JsonValue::Object JsonValue::parseObject(std::string_view json, size_t& pos) {
    pos++; // Skip '{'
    
    Object object;
    skipWhitespace(json, pos);
    
    if (pos < json.length() && json[pos] == '}') {
        pos++;
        return object;
    }
    
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        std::string key = parseString(json, pos);
        skipWhitespace(json, pos);
        
        if (pos >= json.length() || json[pos] != ':') {
            throw std::runtime_error("Expected ':' after object key");
        }
        pos++;
        
        object.emplace(std::move(key), parseValue(json, pos));
        skipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unterminated object");
        }
        
        if (json[pos] == '}') {
            pos++;
            return object;
        }
        
        if (json[pos] != ',') {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        
        pos++;
    }
    
    throw std::runtime_error("Unterminated object");
}

} // namespace n11
