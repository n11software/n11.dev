#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

namespace n11 {

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;
    using ValueType = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool value) : value_(value) {}
    JsonValue(double value) : value_(value) {}
    JsonValue(const std::string& value) : value_(value) {}
    JsonValue(const Array& value) : value_(value) {}
    JsonValue(const Object& value) : value_(value) {}
    JsonValue(int value) : value_(static_cast<double>(value)) {}
    JsonValue(long value) : value_(static_cast<double>(value)) {}
    JsonValue(const char* value) : value_(std::string(value)) {}
    JsonValue(std::initializer_list<JsonValue> arr) : value_(Array(arr)) {}

    static JsonValue parse(const std::string& json);
    std::string stringify() const;

    JsonValue& operator[](const std::string& key) {
        if (auto obj = std::get_if<Object>(&value_)) {
            return (*obj)[key];
        }
        value_ = Object();
        return std::get<Object>(value_)[key];
    }

    JsonValue& operator[](size_t index) {
        if (auto arr = std::get_if<Array>(&value_)) {
            if (index >= arr->size()) {
                throw std::out_of_range("Array index out of bounds");
            }
            return (*arr)[index];
        }
        throw std::runtime_error("Not an array");
    }

    operator std::string() const {
        return stringify();
    }

    JsonValue& operator=(const std::string& value) {
        value_ = value;
        return *this;
    }

    JsonValue& operator=(bool value) {
        value_ = value;
        return *this;
    }

    JsonValue& operator=(double value) {
        value_ = value;
        return *this;
    }

    JsonValue& operator=(const char* value) {
        value_ = std::string(value);
        return *this;
    }

    JsonValue& setRaw(const std::string& raw) {
        value_ = std::string("RAW:") + raw;
        return *this;
    }

    template<typename T>
    JsonValue& operator=(T value) {
        *this = JsonValue(value);
        return *this;
    }

    bool isArray() const {
        return std::holds_alternative<Array>(value_);
    }

    bool isObject() const {
        return std::holds_alternative<Object>(value_);
    }

    bool isNull() const {
        return std::holds_alternative<std::nullptr_t>(value_);
    }

    const Array& getArray() const {
        if (auto arr = std::get_if<Array>(&value_)) {
            return *arr;
        }
        throw std::runtime_error("Not an array");
    }

    const Object& getObject() const {
        if (auto obj = std::get_if<Object>(&value_)) {
            return *obj;
        }
        throw std::runtime_error("Not an object");
    }

    bool hasKey(const std::string& key) const {
        if (auto obj = std::get_if<Object>(&value_)) {
            return obj->find(key) != obj->end();
        }
        return false;
    }

    void push_back(const JsonValue& value) {
        if (!isArray()) {
            value_ = Array();
        }
        std::get<Array>(value_).push_back(value);
    }
    
    bool empty() const {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->empty();
        }
        return true;
    }
    
    size_t size() const {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->size();
        }
        return 0;
    }
    
    Array::iterator begin() {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->begin();
        }
        throw std::runtime_error("Not an array");
    }
    
    Array::iterator end() {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->end();
        }
        throw std::runtime_error("Not an array");
    }

    Array::const_iterator begin() const {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->begin();
        }
        throw std::runtime_error("Not an array");
    }
    
    Array::const_iterator end() const {
        if (auto arr = std::get_if<Array>(&value_)) {
            return arr->end();
        }
        throw std::runtime_error("Not an array");
    }

private:
    ValueType value_;
    
    static JsonValue parseValue(const std::string& json, size_t& pos);
    static std::string parseString(const std::string& json, size_t& pos);
    static double parseNumber(const std::string& json, size_t& pos);
    static Array parseArray(const std::string& json, size_t& pos);
    static Object parseObject(const std::string& json, size_t& pos);
    
    static void skipWhitespace(const std::string& json, size_t& pos);
    static bool isWhitespace(char c);
};

} // namespace n11
