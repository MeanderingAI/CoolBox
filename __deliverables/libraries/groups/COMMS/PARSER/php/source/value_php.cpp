#include "value_php.h"

#include <cmath>
#include <sstream>

namespace plphp {

Value Value::null_value() { Value v; v.type = ValueType::Null; return v; }
Value Value::bool_value(bool b) { Value v; v.type = ValueType::Bool; v.b = b; return v; }
Value Value::int_value(long long i) { Value v; v.type = ValueType::Int; v.i = i; return v; }
Value Value::double_value(double d) { Value v; v.type = ValueType::Double; v.d = d; return v; }
Value Value::string_value(std::string s) { Value v; v.type = ValueType::String; v.s = std::move(s); return v; }

Value Value::array_value() {
    Value v;
    v.type = ValueType::Array;
    v.arr = std::make_shared<std::vector<std::pair<Value, Value>>>();
    v.next_index = 0;
    return v;
}

bool Value::to_bool() const {
    switch (type) {
        case ValueType::Null: return false;
        case ValueType::Bool: return b;
        case ValueType::Int: return i != 0;
        case ValueType::Double: return d != 0.0;
        case ValueType::String: return !s.empty() && s != "0";
        case ValueType::Array: return arr && !arr->empty();
    }
    return false;
}

long long Value::to_int() const {
    switch (type) {
        case ValueType::Null: return 0;
        case ValueType::Bool: return b ? 1 : 0;
        case ValueType::Int: return i;
        case ValueType::Double: return static_cast<long long>(d);
        case ValueType::String:
            try { return std::stoll(s); } catch (...) { return 0; }
        case ValueType::Array: return arr ? static_cast<long long>(arr->size()) : 0;
    }
    return 0;
}

double Value::to_double() const {
    switch (type) {
        case ValueType::Null: return 0.0;
        case ValueType::Bool: return b ? 1.0 : 0.0;
        case ValueType::Int: return static_cast<double>(i);
        case ValueType::Double: return d;
        case ValueType::String:
            try { return std::stod(s); } catch (...) { return 0.0; }
        case ValueType::Array: return arr ? static_cast<double>(arr->size()) : 0.0;
    }
    return 0.0;
}

std::string Value::to_string() const {
    switch (type) {
        case ValueType::Null: return "";
        case ValueType::Bool: return b ? "1" : "";
        case ValueType::Int: return std::to_string(i);
        case ValueType::Double: {
            std::ostringstream oss;
            oss << d;
            return oss.str();
        }
        case ValueType::String: return s;
        case ValueType::Array: return "Array";
    }
    return "";
}

Value* Value::find(const Value& key) const {
    if (!arr) return nullptr;
    const std::string key_str = key.to_string();
    for (auto& kv : *arr) {
        if (kv.first.to_string() == key_str) return &kv.second;
    }
    return nullptr;
}

void Value::set(const Value& key, const Value& value) {
    if (!arr) return;
    const std::string key_str = key.to_string();
    for (auto& kv : *arr) {
        if (kv.first.to_string() == key_str) { kv.second = value; return; }
    }
    arr->emplace_back(key, value);
    if (key.type == ValueType::Int && key.i >= next_index) {
        next_index = key.i + 1;
    }
}

void Value::push(const Value& value) {
    if (!arr) return;
    set(Value::int_value(next_index), value);
}

bool Value::loose_equals(const Value& other) const {
    if (type == ValueType::Array || other.type == ValueType::Array) {
        if (type != other.type || !arr || !other.arr || arr->size() != other.arr->size()) return false;
        for (auto& kv : *arr) {
            const Value* rhs = other.find(kv.first);
            if (!rhs || !kv.second.loose_equals(*rhs)) return false;
        }
        return true;
    }
    if (type == ValueType::String && other.type == ValueType::String) return s == other.s;
    return to_double() == other.to_double();
}

Value clone(const Value& value) {
    Value copy = value;
    if (value.type == ValueType::Array && value.arr) {
        copy.arr = std::make_shared<std::vector<std::pair<Value, Value>>>();
        copy.arr->reserve(value.arr->size());
        for (const auto& kv : *value.arr) {
            copy.arr->emplace_back(clone(kv.first), clone(kv.second));
        }
    }
    return copy;
}

} // namespace plphp
