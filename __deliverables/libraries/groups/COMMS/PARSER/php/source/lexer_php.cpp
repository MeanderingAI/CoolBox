#include "lexer_php.h"

#include <cctype>
#include <unordered_set>

namespace plphp {

namespace {
const std::unordered_set<std::string> kKeywords = {
    "echo", "print", "if", "elseif", "else", "while", "for", "foreach", "as",
    "function", "return", "break", "continue", "global", "true", "false", "null", "array"
};

const std::vector<std::string> kMultiCharSymbols = {
    "===", "!==", "<=>", "==", "!=", "<=", ">=", "&&", "||",
    "+=", "-=", ".=", "*=", "/=", "%=", "=>", "->", "++", "--", "::"
};
} // namespace

LexerPHP::LexerPHP(const std::string& src) : src_(src) {}

char LexerPHP::peek(size_t offset) const {
    const size_t idx = i_ + offset;
    return idx < src_.size() ? src_[idx] : '\0';
}

char LexerPHP::get() {
    const char c = peek();
    if (c == '\n') ++line_;
    ++i_;
    return c;
}

bool LexerPHP::starts_with(const std::string& text) const {
    return src_.compare(i_, text.size(), text) == 0;
}

bool LexerPHP::is_ident_start(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool LexerPHP::is_ident_char(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

void LexerPHP::skip_space() {
    while (true) {
        const char c = peek();
        if (c == '\0') return;
        if (std::isspace(static_cast<unsigned char>(c)) != 0) { get(); continue; }
        if (c == '#') { while (peek() != '\n' && peek() != '\0') get(); continue; }
        if (c == '/' && peek(1) == '/') { while (peek() != '\n' && peek() != '\0') get(); continue; }
        if (c == '/' && peek(1) == '*') {
            get(); get();
            while (!(peek() == '*' && peek(1) == '/') && peek() != '\0') get();
            if (peek() != '\0') { get(); get(); }
            continue;
        }
        return;
    }
}

std::vector<TokenPHP> LexerPHP::tokenize() {
    std::vector<TokenPHP> tokens;
    bool in_php = false;

    while (i_ < src_.size()) {
        if (!in_php) {
            std::string html;
            while (i_ < src_.size() && !starts_with("<?php") && !starts_with("<?=")) {
                html.push_back(get());
            }
            if (!html.empty()) {
                tokens.push_back({TokenTypePHP::InlineHtml, html, line_, false});
            }
            if (starts_with("<?php")) {
                for (int k = 0; k < 5; ++k) get();
                in_php = true;
            } else if (starts_with("<?=")) {
                for (int k = 0; k < 3; ++k) get();
                in_php = true;
                tokens.push_back({TokenTypePHP::Keyword, "echo", line_, false});
            }
            continue;
        }

        skip_space();
        if (i_ >= src_.size()) break;

        if (starts_with("?>")) {
            get(); get();
            tokens.push_back({TokenTypePHP::Symbol, ";", line_, false});
            in_php = false;
            continue;
        }

        const char c = peek();

        if (c == '$') {
            get();
            std::string name;
            while (is_ident_char(peek())) name.push_back(get());
            tokens.push_back({TokenTypePHP::Variable, name, line_, false});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
            std::string num;
            bool is_double = false;
            while (std::isdigit(static_cast<unsigned char>(peek())) != 0) num.push_back(get());
            if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1))) != 0) {
                is_double = true;
                num.push_back(get());
                while (std::isdigit(static_cast<unsigned char>(peek())) != 0) num.push_back(get());
            }
            tokens.push_back({is_double ? TokenTypePHP::Double : TokenTypePHP::Int, num, line_, false});
            continue;
        }

        if (is_ident_start(c)) {
            std::string ident;
            while (is_ident_char(peek())) ident.push_back(get());
            std::string lower = ident;
            for (auto& ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (kKeywords.count(lower) != 0) {
                tokens.push_back({TokenTypePHP::Keyword, lower, line_, false});
            } else {
                tokens.push_back({TokenTypePHP::Identifier, ident, line_, false});
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            const char quote = get();
            std::string str;
            while (peek() != quote && peek() != '\0') {
                char ch = get();
                if (ch == '\\' && quote == '"') {
                    const char next = get();
                    switch (next) {
                        case 'n': str.push_back('\n'); break;
                        case 't': str.push_back('\t'); break;
                        case 'r': str.push_back('\r'); break;
                        case '"': str.push_back('"'); break;
                        case '\\': str.push_back('\\'); break;
                        case '$': str.push_back('$'); break;
                        default: str.push_back('\\'); str.push_back(next); break;
                    }
                } else if (ch == '\\' && quote == '\'') {
                    const char next = get();
                    if (next == '\'' || next == '\\') {
                        str.push_back(next);
                    } else {
                        str.push_back('\\');
                        str.push_back(next);
                    }
                } else {
                    str.push_back(ch);
                }
            }
            if (peek() == quote) get();
            TokenPHP tok{TokenTypePHP::String, str, line_, quote == '"'};
            tokens.push_back(tok);
            continue;
        }

        bool matched = false;
        for (const auto& sym : kMultiCharSymbols) {
            if (starts_with(sym)) {
                for (size_t k = 0; k < sym.size(); ++k) get();
                tokens.push_back({TokenTypePHP::Symbol, sym, line_, false});
                matched = true;
                break;
            }
        }
        if (matched) continue;

        const std::string single(1, get());
        tokens.push_back({TokenTypePHP::Symbol, single, line_, false});
    }

    tokens.push_back({TokenTypePHP::End, "", line_, false});
    return tokens;
}

} // namespace plphp
