#include "pdf_reader.h"
#include "inflate.h"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace docs {

namespace {

std::string read_literal_string(const std::string& content, size_t& idx) {
    std::string result;
    int depth = 1;
    ++idx; // skip '('
    while (idx < content.size() && depth > 0) {
        const char c = content[idx];
        if (c == '\\' && idx + 1 < content.size()) {
            const char next = content[idx + 1];
            switch (next) {
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case '(': result.push_back('('); break;
                case ')': result.push_back(')'); break;
                case '\\': result.push_back('\\'); break;
                default: result.push_back(next); break;
            }
            idx += 2;
            continue;
        }
        if (c == '(') { ++depth; result.push_back(c); ++idx; continue; }
        if (c == ')') { --depth; if (depth > 0) result.push_back(c); ++idx; continue; }
        result.push_back(c);
        ++idx;
    }
    return result;
}

std::string read_hex_string(const std::string& content, size_t& idx) {
    ++idx; // skip '<'
    std::string hex;
    while (idx < content.size() && content[idx] != '>') { hex.push_back(content[idx]); ++idx; }
    if (idx < content.size()) ++idx; // skip '>'

    auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 0;
    };
    std::string result;
    for (size_t k = 0; k + 1 < hex.size(); k += 2) {
        result.push_back(static_cast<char>((hex_val(hex[k]) << 4) | hex_val(hex[k + 1])));
    }
    return result;
}

// Reads the show-text operators (Tj/TJ/'/") out of one decoded page content
// stream, emitting one output line per text-positioning move (Td/TD/T*/ET).
std::string extract_text_from_content_stream(const std::string& content) {
    std::string out;
    std::string current_line;
    size_t i = 0;

    auto flush_line = [&]() {
        if (!current_line.empty()) {
            out += current_line;
            out += '\n';
            current_line.clear();
        }
    };

    while (i < content.size()) {
        const char c = content[i];

        if (c == '(') {
            current_line += read_literal_string(content, i);
            continue;
        }

        if (c == '<') {
            if (i + 1 < content.size() && content[i + 1] == '<') {
                // Skip an embedded dictionary (rare inside content streams).
                i += 2;
                int depth = 1;
                while (i < content.size() && depth > 0) {
                    if (content[i] == '<' && i + 1 < content.size() && content[i + 1] == '<') { depth++; i += 2; continue; }
                    if (content[i] == '>' && i + 1 < content.size() && content[i + 1] == '>') { depth--; i += 2; continue; }
                    ++i;
                }
                continue;
            }
            current_line += read_hex_string(content, i);
            continue;
        }

        if (c == '[') {
            ++i;
            while (i < content.size() && content[i] != ']') {
                if (content[i] == '(') {
                    current_line += read_literal_string(content, i);
                } else if (content[i] == '<') {
                    current_line += read_hex_string(content, i);
                } else if (content[i] == '-' || std::isdigit(static_cast<unsigned char>(content[i])) != 0) {
                    const size_t start = i;
                    if (content[i] == '-') ++i;
                    while (i < content.size() && (std::isdigit(static_cast<unsigned char>(content[i])) != 0 || content[i] == '.')) ++i;
                    try {
                        // Large negative kerning values in a TJ array represent real word gaps.
                        if (std::stod(content.substr(start, i - start)) < -100.0) current_line += ' ';
                    } catch (...) {
                        // ignore malformed numbers
                    }
                } else {
                    ++i;
                }
            }
            if (i < content.size()) ++i; // skip ']'
            continue;
        }

        if (c == '\'' || c == '"') {
            flush_line();
            ++i;
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '*') {
            const size_t start = i;
            while (i < content.size() && (std::isalnum(static_cast<unsigned char>(content[i])) != 0 || content[i] == '*')) ++i;
            const std::string op = content.substr(start, i - start);
            if (op == "Td" || op == "TD" || op == "T*" || op == "ET") flush_line();
            continue;
        }

        ++i;
    }

    flush_line();
    return out;
}

} // namespace

std::string extract_pdf_text_from_bytes(const std::vector<unsigned char>& pdf_bytes) {
    const std::string data(reinterpret_cast<const char*>(pdf_bytes.data()), pdf_bytes.size());
    std::string all_text;

    size_t search_from = 0;
    while (true) {
        const size_t stream_kw = data.find("stream", search_from);
        if (stream_kw == std::string::npos) break;
        const size_t endstream_kw = data.find("endstream", stream_kw);
        if (endstream_kw == std::string::npos) break;

        const size_t dict_start = data.rfind("<<", stream_kw);
        const std::string dict = (dict_start != std::string::npos) ? data.substr(dict_start, stream_kw - dict_start) : std::string();
        const bool is_flate = dict.find("/FlateDecode") != std::string::npos;

        size_t body_start = stream_kw + 6; // length of "stream"
        if (body_start < data.size() && data[body_start] == '\r') ++body_start;
        if (body_start < data.size() && data[body_start] == '\n') ++body_start;
        size_t body_end = endstream_kw;
        while (body_end > body_start && (data[body_end - 1] == '\r' || data[body_end - 1] == '\n')) --body_end;

        std::string decoded;
        if (body_end > body_start) {
            const std::vector<unsigned char> raw(pdf_bytes.begin() + static_cast<long>(body_start),
                                                  pdf_bytes.begin() + static_cast<long>(body_end));
            if (is_flate) {
                zlib_inflate(raw, decoded);
            } else {
                decoded.assign(raw.begin(), raw.end());
            }
        }

        if (decoded.find("BT") != std::string::npos &&
            (decoded.find("Tj") != std::string::npos || decoded.find("TJ") != std::string::npos)) {
            all_text += extract_text_from_content_stream(decoded);
        }

        search_from = endstream_kw + 9; // length of "endstream"
    }

    return all_text;
}

std::string extract_pdf_text(const std::string& pdf_path) {
    std::ifstream file(pdf_path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("cannot open PDF file: " + pdf_path);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return extract_pdf_text_from_bytes(bytes);
}

} // namespace docs
