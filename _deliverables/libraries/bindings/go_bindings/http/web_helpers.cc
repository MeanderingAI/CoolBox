#include "rest_api/forms.h"
#include "rest_api/server.h"
#include "json.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <regex>
#include <set>

namespace ml {
namespace rest_api {
namespace {
using dataformats::json::Array;
using dataformats::json::Object;
using dataformats::json::Parser;
using dataformats::json::Value;

std::string read_text(const std::filesystem::path& path) {
    if (path.string().find('\0') != std::string::npos) {
        throw std::invalid_argument("File path cannot contain NUL");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open file: " + path.string());
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) throw std::runtime_error("Cannot read file: " + path.string());
    return contents.str();
}

std::string read_html(const std::string& root, const std::string& filename) {
    if (root.find('\0') != std::string::npos || filename.find('\0') != std::string::npos) {
        throw std::invalid_argument("HTML paths cannot contain NUL");
    }
    namespace fs = std::filesystem;
    const fs::path relative(filename);
    if (filename.empty() || relative.is_absolute() || relative.has_root_name()) {
        throw std::invalid_argument("HTML filename must be relative to its root");
    }
    const auto base = fs::canonical(root);
    const auto path = fs::canonical(base / relative);
    auto component = path.begin();
    for (const auto& part : base) {
        if (component == path.end() || *component++ != part) {
            throw std::invalid_argument("HTML file is outside its root");
        }
    }
    if (!fs::is_regular_file(path)) throw std::invalid_argument("HTML path is not a regular file");
    return read_text(path);
}

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

std::string media_type(std::string type) {
    type = trim(type.substr(0, type.find(';')));
    std::transform(type.begin(), type.end(), type.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return type;
}

Object object(const Value& value, const std::string& description) {
    if (!value.is_object()) throw std::invalid_argument(description + " must be a JSON object");
    return value.as_object();
}

void known_keys(const Object& value, const std::set<std::string>& allowed) {
    for (const auto& entry : value) {
        if (!allowed.count(entry.first)) throw std::invalid_argument("Unknown schema property: " + entry.first);
    }
}

bool text_type(const std::string& type) { return type == "string" || type == "email"; }

std::string field_type(const Object& field) {
    if (!field.get("type").is_string()) throw std::invalid_argument("Field type must be a string");
    const auto type = field.get("type").as_string();
    if (!text_type(type) && type != "integer" && type != "number" && type != "boolean") {
        throw std::invalid_argument("Unsupported field type: " + type);
    }
    return type;
}

Object schema_fields(const std::string& definition) {
    const auto schema = object(Parser::parse(definition), "Form schema");
    known_keys(schema, {"fields"});
    const auto fields = object(schema.get("fields"), "Form fields");
    for (const auto& [name, value] : fields) {
        if (name.empty()) throw std::invalid_argument("Field name cannot be empty");
        const auto field = object(value, "Field " + name);
        known_keys(field, {"type", "required", "min_length", "max_length", "minimum", "maximum", "enum"});
        const auto type = field_type(field);
        if (field.has("required") && !field.get("required").is_bool()) {
            throw std::invalid_argument("required must be boolean");
        }
        for (const auto* key : {"min_length", "max_length", "minimum", "maximum"}) {
            if (!field.has(key)) continue;
            const auto rule = field.get(key);
            if (!rule.is_number()) throw std::invalid_argument(std::string(key) + " must be numeric");
            const bool length = std::string(key).find("length") != std::string::npos;
            if (length && (!text_type(type) || rule.as_number() < 0 ||
                           std::floor(rule.as_number()) != rule.as_number())) {
                throw std::invalid_argument("Length bounds require a string type and nonnegative integers");
            }
            if (!length && type != "integer" && type != "number") {
                throw std::invalid_argument("Numeric bounds require an integer or number field");
            }
        }
        for (const auto& bounds : {std::make_pair("min_length", "max_length"),
                                  std::make_pair("minimum", "maximum")}) {
            if (field.has(bounds.first) && field.has(bounds.second) &&
                field.get(bounds.first).as_number() > field.get(bounds.second).as_number()) {
                throw std::invalid_argument("Minimum cannot exceed maximum");
            }
        }
        if (field.has("enum")) {
            const auto choices = field.get("enum");
            if (!choices.is_array() || choices.as_array().empty()) {
                throw std::invalid_argument("enum must be a nonempty array");
            }
            for (const auto& choice : choices.as_array()) {
                const bool correct = text_type(type) ? choice.is_string() :
                    type == "boolean" ? choice.is_bool() : choice.is_number();
                if (!correct || (type == "integer" &&
                    (std::floor(choice.as_number()) != choice.as_number() ||
                     std::abs(choice.as_number()) > 9007199254740991.0))) {
                    throw std::invalid_argument("enum values must match the field type");
                }
            }
        }
    }
    return fields;
}

std::string decode_component(const std::string& value) {
    std::string result;
    const auto digit = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') result += ' ';
        else if (value[i] == '%') {
            if (i + 2 >= value.size() || digit(value[i + 1]) < 0 || digit(value[i + 2]) < 0) {
                throw std::invalid_argument("Invalid percent escape in form body");
            }
            result += static_cast<char>(digit(value[i + 1]) * 16 + digit(value[i + 2]));
            i += 2;
        } else result += value[i];
    }
    return result;
}

Object submitted_fields(const std::string& body, const std::string& content_type) {
    const auto type = media_type(content_type);
    if (type == "application/json") return object(Parser::parse(body), "Form submission");
    if (type != "application/x-www-form-urlencoded") {
        throw std::invalid_argument("Unsupported form Content-Type: " + type);
    }
    Object values;
    size_t begin = 0;
    while (begin < body.size()) {
        const auto end = body.find('&', begin);
        const auto pair = body.substr(begin, end == std::string::npos ? end : end - begin);
        if (!pair.empty()) {
            const auto equals = pair.find('=');
            const auto key = decode_component(pair.substr(0, equals));
            if (values.has(key)) throw std::invalid_argument("Duplicate submitted field: " + key);
            values.set(key, decode_component(equals == std::string::npos ? "" : pair.substr(equals + 1)));
        }
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return values;
}

size_t utf8_length(const std::string& text) {
    size_t length = 0;
    for (size_t i = 0; i < text.size(); ++length) {
        const unsigned char ch = text[i++];
        if (ch < 0x80) continue;
        unsigned remaining;
        unsigned code;
        unsigned minimum;
        if (ch >= 0xc2 && ch <= 0xdf) { remaining = 1; code = ch & 0x1f; minimum = 0x80; }
        else if (ch >= 0xe0 && ch <= 0xef) { remaining = 2; code = ch & 0xf; minimum = 0x800; }
        else if (ch >= 0xf0 && ch <= 0xf4) { remaining = 3; code = ch & 7; minimum = 0x10000; }
        else throw std::invalid_argument("Invalid UTF-8 string");
        while (remaining--) {
            if (i >= text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80) {
                throw std::invalid_argument("Invalid UTF-8 string");
            }
            code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) {
            throw std::invalid_argument("Invalid UTF-8 string");
        }
    }
    return length;
}

std::string escape_html(const std::string& text) {
    std::string output;
    for (char ch : text) {
        switch (ch) {
            case '&': output += "&amp;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '"': output += "&quot;"; break;
            case '\'': output += "&#39;"; break;
            default: output += ch;
        }
    }
    return output;
}
} // namespace

FormSchema::FormSchema(const std::string& definition) : definition_(definition) {
    schema_fields(definition_);
}

FormSchema FormSchema::from_file(const std::string& filename) {
    return FormSchema(read_text(filename));
}

ValidationResult FormSchema::validate(const std::string& body, const std::string& content_type) const {
    const auto fields = schema_fields(definition_);
    const auto submitted = submitted_fields(body, content_type);
    ValidationResult result;
    Object normalized;
    for (const auto& [name, value] : submitted) {
        if (!fields.has(name)) result.errors[name].push_back("unknown");
    }
    for (const auto& [name, rule] : fields) {
        const auto field = rule.as_object();
        const auto type = field_type(field);
        Value value = submitted.get(name);
        const bool missing = !submitted.has(name) || value.is_null() ||
            (value.is_string() && trim(value.as_string()).empty());
        if (missing) {
            if (field.has("required") && field.get("required").as_bool()) {
                result.errors[name].push_back("required");
            }
            continue;
        }
        if (type == "integer" || type == "number") {
            if (value.is_string()) {
                const auto text = trim(value.as_string());
                static const std::regex numeric("-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?");
                if (text.size() <= 512 && std::regex_match(text, numeric)) {
                    std::istringstream input(text);
                    input.imbue(std::locale::classic());
                    double number = 0;
                    input >> number;
                    if (input && std::isfinite(number)) value = Value(number);
                }
            }
            if (!value.is_number() || (type == "integer" &&
                (std::floor(value.as_number()) != value.as_number() ||
                 std::abs(value.as_number()) > 9007199254740991.0))) {
                result.errors[name].push_back("type");
                continue;
            }
            if (field.has("minimum") && value.as_number() < field.get("minimum").as_number()) {
                result.errors[name].push_back("minimum");
            }
            if (field.has("maximum") && value.as_number() > field.get("maximum").as_number()) {
                result.errors[name].push_back("maximum");
            }
        } else if (type == "boolean") {
            if (value.is_string()) {
                const auto text = trim(value.as_string());
                if (text == "true" || text == "1" || text == "on") value = Value(true);
                else if (text == "false" || text == "0" || text == "off") value = Value(false);
            }
            if (!value.is_bool()) { result.errors[name].push_back("type"); continue; }
        } else {
            if (!value.is_string()) { result.errors[name].push_back("type"); continue; }
            const auto text = value.as_string();
            const auto length = utf8_length(text);
            if (field.has("min_length") && length < field.get("min_length").as_number()) {
                result.errors[name].push_back("min_length");
            }
            if (field.has("max_length") && length > field.get("max_length").as_number()) {
                result.errors[name].push_back("max_length");
            }
            if (type == "email") {
                const auto at = text.find('@');
                const auto domain = at == std::string::npos ? "" : text.substr(at + 1);
                if (text.size() > 254 || at == std::string::npos || at == 0 ||
                    text.find('@', at + 1) != std::string::npos ||
                    std::any_of(text.begin(), text.end(), [](unsigned char c) { return c <= 32 || c == 127; }) ||
                    text.find_first_of("<>") != std::string::npos ||
                    domain.empty() || domain.front() == '.' || domain.back() == '.' ||
                    domain.find('.') == std::string::npos || domain.find("..") != std::string::npos) {
                    result.errors[name].push_back("email");
                }
            }
        }
        if (field.has("enum")) {
            bool found = false;
            for (const auto& choice : field.get("enum").as_array()) {
                if (choice.to_string() == value.to_string()) found = true;
            }
            if (!found) result.errors[name].push_back("enum");
        }
        if (!result.errors.count(name)) normalized.set(name, value);
    }
    result.valid = result.errors.empty();
    result.values_json = normalized.to_string();
    return result;
}

std::string ValidationResult::to_json() const {
    Object output, error_object;
    for (const auto& [name, messages] : errors) error_object.set(name, Array(messages.begin(), messages.end()));
    output.set("valid", valid);
    output.set("values", Parser::parse(values_json));
    output.set("errors", error_object);
    return output.to_string();
}

Response Response::html(const std::string& content, int status) {
    Response response(status, content);
    response.set_header("Content-Type", "text/html; charset=utf-8");
    return response;
}

Response Response::html_file(const std::string& root, const std::string& filename, int status) {
    return html(read_html(root, filename), status);
}

Response Response::render(const std::string& root, const std::string& filename,
                          const std::map<std::string, std::string>& context, int status) {
    const auto source = read_html(root, filename);
    std::string rendered;
    size_t begin = 0;
    while (begin < source.size()) {
        const auto open = source.find("{{", begin);
        if (open == std::string::npos) { rendered += source.substr(begin); break; }
        rendered += source.substr(begin, open - begin);
        const auto close = source.find("}}", open + 2);
        if (close == std::string::npos) throw std::invalid_argument("Unterminated template placeholder");
        const auto key = trim(source.substr(open + 2, close - open - 2));
        const auto entry = context.find(key);
        if (entry == context.end()) throw std::invalid_argument("Missing template value: " + key);
        rendered += escape_html(entry->second);
        begin = close + 2;
    }
    return html(rendered, status);
}

} // namespace rest_api
} // namespace ml
