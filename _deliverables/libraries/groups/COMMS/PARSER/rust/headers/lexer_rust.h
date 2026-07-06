#pragma once
#include <string>
#include <vector>

namespace prust {

struct Token {
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &src): src(src) {}
    std::vector<Token> tokenize() const;
private:
    std::string src;
};

}
