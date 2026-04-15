#pragma once
#include <string>
#include <vector>

namespace ppython {
struct Token { std::string text; };
class Lexer { public: explicit Lexer(const std::string &s): src(s) {} std::vector<Token> tokenize() const; private: std::string src; };
}
