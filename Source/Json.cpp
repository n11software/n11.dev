#include "Json.hpp"
#include <sstream>
#include <iomanip>

namespace n11 {

JsonValue JsonValue::parse(const std::string& json) {
    size_t pos = 0;
    skipWhitespace(json, pos);
    return parseValue(json, pos);
}

JsonValue JsonValue::parseValue(const std::string& json, size_t& pos) {
    skipWhitespace(json, pos);
    
    if (pos >= json.length()) {
        throw std::runtime_error("Unexpected end of input");
    }

    switch (json[pos]) {
        case 'n':
            if (json.substr(pos, 4) == "null") {
                pos += 4;
                return JsonValue(nullptr);
            }
            break;
        case 't':
            if (json.substr(pos, 4) == "true") {
                pos += 4;
                return JsonValue(true);
            }
            break;
        case 'f':
            if (json.substr(pos, 5) == "false") {
                pos += 5;
                return JsonValue(false);
            }
            break;
        case '"':
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
        } else if constexpr (std::is_same_v<T, std::string>) {
            oss << '"';
            for (char c : v) {
                switch (c) {
                    case '"': oss << "\\\""; break;
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
        } else if constexpr (std::is_same_v<T, Array>) {
            oss << '[';
            bool first = true;
            for (const auto& elem : v) {
                if (!first) oss << ',';
                oss << elem.stringify();
                first = false;
            }
            oss << ']';
        } else if constexpr (std::is_same_v<T, Object>) {
            oss << '{';
            bool first = true;
            for (const auto& [key, value] : v) {
                if (!first) oss << ',';
                oss << '"' << key << "\":" << value.stringify();
                first = false;
            }
            oss << '}';
        }
    }, value_);
    
    return oss.str();
}

// Helper parsing functions implementation would go here
// ...

std::string JsonValue::parseString(const std::string& json, size_t& pos) {
    if (json[pos] != '"') {
        throw std::runtime_error("Expected '\"' at start of string");
    }
    
    std::string result;
    pos++; // Skip opening quote
    
    while (pos < json.length()) {
        char c = json[pos++];
        if (c == '"') {
            return result;
        }
        if (c == '\\' && pos < json.length()) {
            char next = json[pos++];
            switch (next) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += next;
            }
        } else {
            result += c;
        }
    }
    
    throw std::runtime_error("Unterminated string");
}

double JsonValue::parseNumber(const std::string& json, size_t& pos) {
    size_t start = pos;
    if (json[pos] == '-') pos++;
    
    while (pos < json.length() && std::isdigit(json[pos])) pos++;
    if (pos < json.length() && json[pos] == '.') {
        pos++;
        while (pos < json.length() && std::isdigit(json[pos])) pos++;
    }
    
    std::string numStr = json.substr(start, pos - start);
    return std::stod(numStr);
}

void JsonValue::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.length() && isWhitespace(json[pos])) {
        pos++;
    }
}

bool JsonValue::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

JsonValue::Array JsonValue::parseArray(const std::string& json, size_t& pos) {
    if (json[pos] != '[') {
        throw std::runtime_error("Expected '[' at start of array");
    }
    pos++; // Skip '['
    
    Array array;
    skipWhitespace(json, pos);
    
    if (pos < json.length() && json[pos] == ']') {
        pos++; // Skip ']'
        return array;
    }
    
    while (pos < json.length()) {
        array.push_back(parseValue(json, pos));
        skipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unterminated array");
        }
        
        if (json[pos] == ']') {
            pos++; // Skip ']'
            return array;
        }
        
        if (json[pos] != ',') {
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        
        pos++; // Skip ','
        skipWhitespace(json, pos);
    }
    
    throw std::runtime_error("Unterminated array");
}

JsonValue::Object JsonValue::parseObject(const std::string& json, size_t& pos) {
    if (json[pos] != '{') {
        throw std::runtime_error("Expected '{' at start of object");
    }
    pos++; // Skip '{'
    
    Object object;
    skipWhitespace(json, pos);
    
    if (pos < json.length() && json[pos] == '}') {
        pos++; // Skip '}'
        return object;
    }
    
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        std::string key = parseString(json, pos);
        skipWhitespace(json, pos);
        
        if (pos >= json.length() || json[pos] != ':') {
            throw std::runtime_error("Expected ':' after object key");
        }
        pos++; // Skip ':'
        
        object[key] = parseValue(json, pos);
        skipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unterminated object");
        }
        
        if (json[pos] == '}') {
            pos++; // Skip '}'
            return object;
        }
        
        if (json[pos] != ',') {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        
        pos++; // Skip ','
    }
    
    throw std::runtime_error("Unterminated object");
}

} // namespace n11
