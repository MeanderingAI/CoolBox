#pragma once
#include <memory>
#include <string>
#include <vector>

namespace plphp {

enum class ValueType { Null, Bool, Int, Double, String, Array };

// PHP-like dynamically typed value. Arrays are ordered key/value lists
// (mirrors PHP's ordered associative arrays) stored behind a shared_ptr;
// use clone() to get copy-on-assign semantics like real PHP arrays.
class Value {
public:
    ValueType type = ValueType::Null;
    bool b = false;
    long long i = 0;
    double d = 0.0;
    std::string s;
    std::shared_ptr<std::vector<std::pair<Value, Value>>> arr;
    long long next_index = 0;

    Value() = default;

    static Value null_value();
    static Value bool_value(bool v);
    static Value int_value(long long v);
    static Value double_value(double v);
    static Value string_value(std::string v);
    static Value array_value();

    bool to_bool() const;
    long long to_int() const;
    double to_double() const;
    std::string to_string() const;

    bool is_array() const { return type == ValueType::Array; }

    Value* find(const Value& key) const;
    void set(const Value& key, const Value& value);
    void push(const Value& value);

    bool loose_equals(const Value& other) const;
};

// Deep-copies arrays so assignment mimics PHP value semantics.
Value clone(const Value& value);

} // namespace plphp
