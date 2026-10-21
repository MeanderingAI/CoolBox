
#include "json.h"
#include <sstream>
#include <regex>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <limits>

namespace dataformats {
namespace json {

// =========================================================================
// Value Implementation
// =========================================================================

Value::Value() : type_(Type::NULL_VALUE), data_(std::monostate{}) {}

Value::Value(std::nullptr_t) : type_(Type::NULL_VALUE), data_(std::monostate{}) {}

Value::Value(bool b) : type_(Type::BOOLEAN), data_(b) {}

Value::Value(int n) : type_(Type::NUMBER), data_(static_cast<double>(n)) {}

Value::Value(double n) : type_(Type::NUMBER), data_(n) {}

Value::Value(const std::string& s) : type_(Type::STRING), data_(s) {}

Value::Value(const char* s) : type_(Type::STRING), data_(std::string(s)) {}

Value::Value(const Array& arr) : type_(Type::ARRAY), data_(std::make_shared<Array>(arr)) {}

Value::Value(const Object& obj) : type_(Type::OBJECT), data_(std::make_shared<Object>(obj)) {}

bool Value::as_bool() const {
    if (type_ != Type::BOOLEAN) {
        throw std::runtime_error("Value is not a boolean");
    }
    return std::get<bool>(data_);
}

double Value::as_number() const {
    if (type_ != Type::NUMBER) {
        throw std::runtime_error("Value is not a number");
    }
    return std::get<double>(data_);
}

std::string Value::as_string() const {
    if (type_ != Type::STRING) {
        throw std::runtime_error("Value is not a string");
    }
    return std::get<std::string>(data_);
}

Array Value::as_array() const {
    if (type_ != Type::ARRAY) {
        throw std::runtime_error("Value is not an array");
    }
    return *std::get<std::shared_ptr<Array>>(data_);
}

Object Value::as_object() const {
    if (type_ != Type::OBJECT) {
        throw std::runtime_error("Value is not an object");
    }
    return *std::get<std::shared_ptr<Object>>(data_);
}

std::string Value::to_string() const {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    
    switch (type_) {
        case Type::NULL_VALUE:
            oss << "null";
            break;
        case Type::BOOLEAN:
            oss << (std::get<bool>(data_) ? "true" : "false");
            break;
        case Type::NUMBER:
            if (!std::isfinite(std::get<double>(data_))) {
                throw std::runtime_error("Cannot serialize a non-finite JSON number");
            }
            oss << std::setprecision(std::numeric_limits<double>::max_digits10);
            oss << std::get<double>(data_);
            break;
        case Type::STRING:
            oss << '"';
            for (unsigned char ch : std::get<std::string>(data_)) {
                switch (ch) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default:
                        if (ch < 0x20) {
                            oss << "\\u00" << std::hex << std::setw(2)
                                << std::setfill('0') << static_cast<int>(ch) << std::dec;
                        } else {
                            oss << ch;
                        }
                }
            }
            oss << '"';
            break;
        case Type::ARRAY:
            oss << std::get<std::shared_ptr<Array>>(data_)->to_string();
            break;
        case Type::OBJECT:
            oss << std::get<std::shared_ptr<Object>>(data_)->to_string();
            break;
    }
    
    return oss.str();
}

// =========================================================================
// Object Implementation
// =========================================================================

void Object::set(const std::string& key, const Value& value) {
    data_[key] = value;
}

Value Object::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return Value();  // Return null
    }
    return it->second;
}

bool Object::has(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void Object::remove(const std::string& key) {
    data_.erase(key);
}

std::vector<std::string> Object::keys() const {
    std::vector<std::string> result;
    for (const auto& [key, value] : data_) {
        result.push_back(key);
    }
    return result;
}

std::string Object::to_string() const {
    std::ostringstream oss;
    oss << "{";
    
    bool first = true;
    for (const auto& [key, value] : data_) {
        if (!first) {
            oss << ", ";
        }
        oss << Value(key).to_string() << ": " << value.to_string();
        first = false;
    }
    
    oss << "}";
    return oss.str();
}

// =========================================================================
// Array Implementation
// =========================================================================

void Array::push(const Value& value) {
    data_.push_back(value);
}

Value Array::get(size_t index) const {
    if (index >= data_.size()) {
        throw std::out_of_range("Array index out of range");
    }
    return data_[index];
}

void Array::set(size_t index, const Value& value) {
    if (index >= data_.size()) {
        throw std::out_of_range("Array index out of range");
    }
    data_[index] = value;
}

std::string Array::to_string() const {
    std::ostringstream oss;
    oss << "[";
    
    for (size_t i = 0; i < data_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << data_[i].to_string();
    }
    
    oss << "]";
    return oss.str();
}

// =========================================================================
// Parser Implementation
// =========================================================================

void Parser::skip_whitespace(std::istringstream& iss) {
    while (iss && (iss.peek() == ' ' || iss.peek() == '\t' ||
                   iss.peek() == '\r' || iss.peek() == '\n')) {
        iss.get();
    }
}

Value Parser::parse(const std::string& json) {
    std::istringstream iss(json);
    auto result = parse_value(iss);
    skip_whitespace(iss);
    if (iss.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("Unexpected trailing JSON content");
    }
    return result;
}

Value Parser::parse_value(std::istringstream& iss) {
    // Bound recursive object/array parsing for untrusted request bodies.
    static thread_local unsigned depth = 0;
    if (depth >= 64) throw std::runtime_error("JSON nesting limit exceeded");
    struct DepthGuard {
        unsigned& depth;
        explicit DepthGuard(unsigned& value) : depth(value) { ++depth; }
        ~DepthGuard() { --depth; }
    } guard(depth);
    skip_whitespace(iss);
    
    char c = iss.peek();
    
    if (c == '{') {
        return Value(parse_object(iss));
    } else if (c == '[') {
        return Value(parse_array(iss));
    } else if (c == '"') {
        return Value(parse_string(iss));
    } else if (c == 't' || c == 'f') {
        // Boolean
        std::string word;
        while (iss && std::isalpha(iss.peek())) {
            word += iss.get();
        }
        if (word != "true" && word != "false") throw std::runtime_error("Invalid JSON boolean");
        return Value(word == "true");
    } else if (c == 'n') {
        // Null
        std::string word;
        while (iss && std::isalpha(iss.peek())) {
            word += iss.get();
        }
        if (word != "null") throw std::runtime_error("Invalid JSON null");
        return Value();
    } else if ((c >= '0' && c <= '9') || c == '-') {
        return Value(parse_number(iss));
    }
    
    throw std::runtime_error("Invalid JSON");
}

Object Parser::parse_object(std::istringstream& iss) {
    Object obj;
    
    skip_whitespace(iss);
    iss.get();  // Skip '{'
    skip_whitespace(iss);
    
    if (iss.peek() == '}') {
        iss.get();
        return obj;
    }
    
    while (iss) {
        skip_whitespace(iss);
        
        // Parse key
        std::string key = parse_string(iss);
        if (obj.has(key)) throw std::runtime_error("Duplicate JSON object key");
        
        skip_whitespace(iss);
        if (iss.get() != ':') {
            throw std::runtime_error("Expected ':' in object");
        }
        
        // Parse value
        Value value = parse_value(iss);
        obj.set(key, value);
        
        skip_whitespace(iss);
        char next = iss.get();
        
        if (next == '}') {
            break;
        } else if (next != ',') {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
    }
    
    if (!iss) throw std::runtime_error("Unterminated JSON object");
    return obj;
}

Array Parser::parse_array(std::istringstream& iss) {
    Array arr;
    
    skip_whitespace(iss);
    iss.get();  // Skip '['
    skip_whitespace(iss);
    
    if (iss.peek() == ']') {
        iss.get();
        if (!iss) throw std::runtime_error("Unterminated JSON array");
        return arr;
    }
    
    while (iss) {
        Value value = parse_value(iss);
        arr.push(value);
        
        skip_whitespace(iss);
        char next = iss.get();
        
        if (next == ']') {
            break;
        } else if (next != ',') {
            throw std::runtime_error("Expected ',' or ']' in array");
        }
    }
    
    return arr;
}

std::string Parser::parse_string(std::istringstream& iss) {
    skip_whitespace(iss);
    
    if (iss.get() != '"') {
        throw std::runtime_error("Expected '\"' at start of string");
    }
    
    std::string result;
    const auto hex4 = [&iss]() {
        unsigned result = 0;
        for (int i = 0; i < 4; ++i) {
            const int ch = iss.get();
            int digit = ch >= '0' && ch <= '9' ? ch - '0' :
                        ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 :
                        ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
            if (digit < 0) throw std::runtime_error("Invalid JSON Unicode escape");
            result = result * 16 + static_cast<unsigned>(digit);
        }
        return result;
    };
    while (true) {
        const int next = iss.get();
        if (next == std::char_traits<char>::eof()) throw std::runtime_error("Unterminated JSON string");
        const unsigned char c = static_cast<unsigned char>(next);
        if (c == '"') return result;
        if (c < 0x20) throw std::runtime_error("Control character in JSON string");
        if (c == '\\') {
            const int escaped = iss.get();
            switch (escaped) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case '/': result += '/'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'u': {
                    unsigned code = hex4();
                    if (code >= 0xd800 && code <= 0xdbff) {
                        if (iss.get() != '\\' || iss.get() != 'u') {
                            throw std::runtime_error("Missing low Unicode surrogate");
                        }
                        const unsigned low = hex4();
                        if (low < 0xdc00 || low > 0xdfff) throw std::runtime_error("Invalid Unicode surrogate");
                        code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                    } else if (code >= 0xdc00 && code <= 0xdfff) {
                        throw std::runtime_error("Unexpected low Unicode surrogate");
                    }
                    if (code <= 0x7f) result += static_cast<char>(code);
                    else {
                        if (code > 0xffff) result += static_cast<char>(0xf0 | (code >> 18));
                        if (code > 0x7ff) result += static_cast<char>(
                            (code > 0xffff ? 0x80 : 0xe0) | ((code >> 12) & 0x3f));
                        result += static_cast<char>((code > 0x7ff ? 0x80 : 0xc0) | ((code >> 6) & 0x3f));
                        result += static_cast<char>(0x80 | (code & 0x3f));
                    }
                    break;
                }
                default: throw std::runtime_error("Invalid JSON escape");
            }
        } else {
            result += c;
        }
    }
}

double Parser::parse_number(std::istringstream& iss) {
    skip_whitespace(iss);
    std::string token;
    while (iss && (std::isdigit(iss.peek()) || iss.peek() == '-' || iss.peek() == '+' ||
                   iss.peek() == '.' || iss.peek() == 'e' || iss.peek() == 'E')) {
        if (token.size() >= 512) throw std::runtime_error("JSON number is too long");
        token += static_cast<char>(iss.get());
    }
    static const std::regex number("-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?");
    if (!std::regex_match(token, number)) throw std::runtime_error("Invalid JSON number");
    std::istringstream input(token);
    input.imbue(std::locale::classic());
    double result = 0;
    input >> result;
    if (!input || !std::isfinite(result)) throw std::runtime_error("JSON number out of range");
    return result;
}

// =========================================================================
// Simple JSON Utilities
// =========================================================================

namespace simple {

std::string encode(const std::map<std::string, std::string>& data) {
    Object obj;
    for (const auto& [key, value] : data) {
        obj.set(key, Value(value));
    }
    return obj.to_string();
}

std::map<std::string, std::string> decode(const std::string& json) {
    std::map<std::string, std::string> result;
    
    try {
        Value val = Parser::parse(json);
        if (val.is_object()) {
            Object obj = val.as_object();
            for (const auto& [key, value] : obj) {
                if (value.is_string()) {
                    result[key] = value.as_string();
                }
            }
        }
    } catch (...) {
        // Fallback to regex parsing
        std::regex pair_regex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        std::string temp = json;
        
        while (std::regex_search(temp, match, pair_regex)) {
            result[match[1].str()] = match[2].str();
            temp = match.suffix();
        }
    }
    
    return result;
}

std::string encode_array(const std::vector<std::string>& data) {
    Array arr;
    for (const auto& item : data) {
        arr.push(Value(item));
    }
    return arr.to_string();
}

std::vector<std::string> decode_array(const std::string& json) {
    std::vector<std::string> result;
    
    try {
        Value val = Parser::parse(json);
        if (val.is_array()) {
            Array arr = val.as_array();
            for (const auto& item : arr) {
                if (item.is_string()) {
                    result.push_back(item.as_string());
                }
            }
        }
    } catch (...) {
        // Fallback to regex parsing
        std::regex item_regex("\"([^\"]+)\"");
        std::smatch match;
        std::string temp = json;
        
        while (std::regex_search(temp, match, item_regex)) {
            result.push_back(match[1].str());
            temp = match.suffix();
        }
    }
    
    return result;
}

std::string encode_numbers(const std::vector<double>& data) {
    Array arr;
    for (double num : data) {
        arr.push(Value(num));
    }
    return arr.to_string();
}

std::vector<double> decode_numbers(const std::string& json) {
    std::vector<double> result;
    
    try {
        Value val = Parser::parse(json);
        if (val.is_array()) {
            Array arr = val.as_array();
            for (const auto& item : arr) {
                if (item.is_number()) {
                    result.push_back(item.as_number());
                }
            }
        }
    } catch (...) {
        // Fallback: extract numbers
        std::regex num_regex("([0-9.eE+-]+)");
        std::smatch match;
        std::string temp = json;
        
        while (std::regex_search(temp, match, num_regex)) {
            try {
                result.push_back(std::stod(match[1].str()));
            } catch (...) {}
            temp = match.suffix();
        }
    }
    
    return result;
}

} // namespace simple

// =========================================================================
// Builder Implementation
// =========================================================================

Builder& Builder::add(const std::string& key, const Value& value) {
    obj_->set(key, value);
    return *this;
}


Builder& Builder::add(const std::string& key, const std::string& value) {
    Value v(value);
    std::cout << "[DEBUG] Builder::add key='" << key << "', value='" << value << "', type=" << static_cast<int>(v.type()) << std::endl;
    obj_->set(key, v);
    return *this;
}

Builder& Builder::add(const std::string& key, const char* value) {
    Value v{std::string(value)};
    std::cout << "[DEBUG] Builder::add key='" << key << "', value='" << value << "', type=" << static_cast<int>(v.type()) << std::endl;
    obj_->set(key, v);
    return *this;
}

Builder& Builder::add(const std::string& key, double value) {
    obj_->set(key, Value(value));
    return *this;
}

Builder& Builder::add(const std::string& key, int value) {
    obj_->set(key, Value(value));
    return *this;
}

Builder& Builder::add(const std::string& key, bool value) {
    obj_->set(key, Value(value));
    return *this;
}

} // namespace json
} // namespace dataformats
