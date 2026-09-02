#pragma once
#include <string>
#include <vector>

namespace plphp {

enum class TokenTypePHP { InlineHtml, Variable, Identifier, Keyword, Int, Double, String, Symbol, End };

struct TokenPHP {
    TokenTypePHP type;
    std::string text;
    int line = 0;
    bool interpolate = false; // true for double-quoted strings (supports $var substitution)
};

class LexerPHP {
public:
    explicit LexerPHP(const std::string& src);
    std::vector<TokenPHP> tokenize();

private:
    const std::string src_;
    size_t i_ = 0;
    int line_ = 1;

    char peek(size_t offset = 0) const;
    char get();
    bool starts_with(const std::string& text) const;
    void skip_space();
    bool is_ident_start(char c) const;
    bool is_ident_char(char c) const;
};

} // namespace plphp
