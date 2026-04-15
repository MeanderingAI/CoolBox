#include "headers/lexer_python.h"
#include <sstream>

namespace ppython {
std::vector<Token> Lexer::tokenize() const {
    std::vector<Token> out;
    std::istringstream iss(src);
    std::string t;
    while(iss >> t) out.push_back({t});
    return out;
}
}
