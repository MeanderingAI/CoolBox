#ifndef ML_JSON_JSON_H
#define ML_JSON_JSON_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <variant>
#include <sstream>

namespace dataformats {
namespace json {

// Forward declarations
class Value;
class Object;
class Array;

// JSON value types
enum class Type {
    NULL_VALUE,
    BOOLEAN,
    NUMBER,
    STRING,
    ARRAY,
    OBJECT
};

// JSON Value class - can hold any JSON type
class Value {
public:
    Value();
    Value(bool b);
    Value(int n);
    Value(double n);
    Value(const std::string& s);
    Value(const char* s);
    Value(const Array& arr);
    Value(const Object& obj);
    Value(std::nullptr_t);
    
    Type type() const { return type_; }
    
    bool is_null() const { return type_ == Type::NULL_VALUE; }
    bool is_bool() const { return type_ == Type::BOOLEAN; }
    bool is_number() const { return type_ == Type::NUMBER; }
    bool is_string() const { return type_ == Type::STRING; }
    bool is_array() const { return type_ == Type::ARRAY; }
    bool is_object() const { return type_ == Type::OBJECT; }
    
    bool as_bool() const;
    double as_number() const;
    std::string as_string() const;
    Array as_array() const;
    Object as_object() const;
    
    std::string to_string() const;
    
private:
    Type type_;
    std::variant<std::monostate, bool, double, std::string, 
                 std::shared_ptr<Array>, std::shared_ptr<Object>> data_;
    
    friend class Object;
    friend class Array;
};

// JSON Object class
class Object {
public:
    Object() = default;
    
    void set(const std::string& key, const Value& value);
    Value get(const std::string& key) const;
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    
    std::vector<std::string> keys() const;
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    std::string to_string() const;
    
    // Iterator support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
private:
    std::map<std::string, Value> data_;
    
    friend class Value;
};

// JSON Array class
class Array {
public:
    Array() = default;
    template<typename It>
    Array(It begin, It end) {
        for (auto it = begin; it != end; ++it) {
            data_.push_back(Value(*it));
        }
    }
    
    void push(const Value& value);
    Value get(size_t index) const;
    void set(size_t index, const Value& value);
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    std::string to_string() const;
    
    // Iterator support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
private:
    std::vector<Value> data_;
    
    friend class Value;
};

// Parser class
class Parser {
public:
    static Value parse(const std::string& json);
    
private:
    static Value parse_value(std::istringstream& iss);
    static Object parse_object(std::istringstream& iss);
    static Array parse_array(std::istringstream& iss);
    static std::string parse_string(std::istringstream& iss);
    static double parse_number(std::istringstream& iss);
    static void skip_whitespace(std::istringstream& iss);
};

// Utility functions for simple key-value JSON
namespace simple {
    std::string encode(const std::map<std::string, std::string>& data);
    std::map<std::string, std::string> decode(const std::string& json);
    
    std::string encode_array(const std::vector<std::string>& data);
    std::vector<std::string> decode_array(const std::string& json);
    
    std::string encode_numbers(const std::vector<double>& data);
    std::vector<double> decode_numbers(const std::string& json);
}

// Builder pattern for JSON construction
class Builder {
public:
    Builder() : obj_(std::make_shared<Object>()) {}

    Builder& add(const std::string& key, const Value& value);
    Builder& add(const std::string& key, const std::string& value);
    Builder& add(const std::string& key, const char* value); // <-- Add this overload
    Builder& add(const std::string& key, double value);
    Builder& add(const std::string& key, int value);
    Builder& add(const std::string& key, bool value);

    Object build() const { return *obj_; }
    std::string to_string() const { return obj_->to_string(); }

private:
    std::shared_ptr<Object> obj_;
};

} // namespace json
} // namespace dataformats

#endif // ML_JSON_JSON_H
