#pragma once
#include <string>
#include <vector>

namespace pljs {

enum class TokenTypeJS { Identifier, Number, Keyword, String, Symbol, End };

struct TokenJS {
    TokenTypeJS type;
    std::string text;
    int line = 0;
};

class LexerJS {
public:
    explicit LexerJS(const std::string &src);
    std::vector<TokenJS> tokenize();
private:
    const std::string src_;
    size_t i_ = 0;
    int line_ = 1;
    char peek() const;
    char get();
    void skip_space();
    bool is_ident_start(char c) const;
    bool is_ident_char(char c) const;
};

} // namespace pljs
