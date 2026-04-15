#pragma once
#include <string>
#include <vector>

namespace plang {

enum class TokenType { Identifier, Number, Keyword, Symbol, End, Newline };

struct Token {
    TokenType type;
    std::string text;
    int line = 0;
};

class Lexer {
public:
    explicit Lexer(const std::string &src);
    std::vector<Token> tokenize();
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

} // namespace plang
