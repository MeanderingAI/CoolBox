#include "circuit.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>

namespace curcuitry {

// ===================================================================
// Minimal JSON tokeniser / parser
// ===================================================================
// This avoids pulling in a full JSON library.  It handles the subset
// of JSON produced by the circuit-description format:
//   - top-level array of objects
//   - string, number, and (ignored) null values
//
// For production use, swap this out for nlohmann/json or similar.
// ===================================================================

namespace {

// Skip whitespace
void skip_ws(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
        ++pos;
}

// Expect and consume a character
void expect(const std::string& s, size_t& pos, char c) {
    skip_ws(s, pos);
    if (pos >= s.size() || s[pos] != c)
        throw std::runtime_error(
            std::string("JSON parse: expected '") + c +
            "' at position " + std::to_string(pos));
    ++pos;
}

// Parse a JSON string (returns content without quotes)
std::string parse_string(const std::string& s, size_t& pos) {
    skip_ws(s, pos);
    if (pos >= s.size() || s[pos] != '"')
        throw std::runtime_error(
            "JSON parse: expected '\"' at position " + std::to_string(pos));
    ++pos;  // skip opening quote
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\') {
            ++pos;
            if (pos < s.size()) {
                switch (s[pos]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    default:   result += s[pos]; break;
                }
            }
        } else {
            result += s[pos];
        }
        ++pos;
    }
    if (pos >= s.size())
        throw std::runtime_error("JSON parse: unterminated string");
    ++pos;  // skip closing quote
    return result;
}

// Parse a JSON number (integer or float) as a string
std::string parse_number_str(const std::string& s, size_t& pos) {
    skip_ws(s, pos);
    size_t start = pos;
    if (pos < s.size() && s[pos] == '-') ++pos;
    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
    if (pos < s.size() && s[pos] == '.') {
        ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
    }
    if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
        ++pos;
        if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
    }
    return s.substr(start, pos - start);
}

// Parse a JSON value – returns it as a string regardless of type.
// (We only need strings and numbers for our format.)
std::string parse_value(const std::string& s, size_t& pos) {
    skip_ws(s, pos);
    if (pos >= s.size())
        throw std::runtime_error("JSON parse: unexpected end of input");

    if (s[pos] == '"') {
        return parse_string(s, pos);
    }
    if (s[pos] == '-' || std::isdigit(static_cast<unsigned char>(s[pos]))) {
        return parse_number_str(s, pos);
    }
    // null, true, false – skip and return empty
    if (s.compare(pos, 4, "null") == 0) { pos += 4; return ""; }
    if (s.compare(pos, 4, "true") == 0) { pos += 4; return "true"; }
    if (s.compare(pos, 5, "false") == 0) { pos += 5; return "false"; }

    throw std::runtime_error(
        "JSON parse: unexpected character '" + std::string(1, s[pos]) +
        "' at position " + std::to_string(pos));
}

// Parse a JSON object into a key-value map (all values as strings).
std::map<std::string, std::string> parse_object(const std::string& s,
                                                  size_t& pos) {
    std::map<std::string, std::string> obj;
    expect(s, pos, '{');
    skip_ws(s, pos);
    if (pos < s.size() && s[pos] == '}') { ++pos; return obj; }

    while (true) {
        std::string key = parse_string(s, pos);
        skip_ws(s, pos);
        expect(s, pos, ':');
        std::string val = parse_value(s, pos);
        obj[key] = val;

        skip_ws(s, pos);
        if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
        break;
    }
    expect(s, pos, '}');
    return obj;
}

double to_double(const std::string& s, double def = 0.0) {
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

} // anonymous namespace

// ===================================================================
// Circuit::from_json
// ===================================================================

Circuit Circuit::from_json(const std::string& json) {
    Circuit circuit;
    size_t pos = 0;

    expect(json, pos, '[');
    skip_ws(json, pos);
    if (pos < json.size() && json[pos] == ']') { ++pos; return circuit; }

    while (true) {
        auto obj = parse_object(json, pos);

        std::string comp_type = obj["type"];
        double x1 = to_double(obj["x1"]);
        double y1 = to_double(obj["y1"]);
        double x2 = to_double(obj["x2"]);
        double y2 = to_double(obj["y2"]);
        std::string label = obj["label"];
        std::string value = obj["value"];

        if (comp_type == "battery") {
            std::string res_str = obj.count("resistance") ? obj["resistance"] : "";
            Battery bat = Battery::from_fields(x1, y1, x2, y2,
                                               label, value, res_str);
            circuit.add_battery(bat);

        } else if (comp_type == "resistor") {
            Resistor res = Resistor::from_fields(x1, y1, x2, y2,
                                                  label, value);
            circuit.add_resistor(res);

        } else if (comp_type == "wire") {
            Wire w = Wire::from_fields(x1, y1, x2, y2, label, value);
            circuit.add_wire(w);

        } else {
            throw std::runtime_error("Unknown component type: " + comp_type);
        }

        skip_ws(json, pos);
        if (pos < json.size() && json[pos] == ',') { ++pos; continue; }
        break;
    }

    expect(json, pos, ']');
    return circuit;
}

} // namespace curcuitry
