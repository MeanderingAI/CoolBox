#include "headers/lexer_rust.h"
#include <sstream>

namespace prust {

std::vector<Token> Lexer::tokenize() const {
    std::vector<Token> out;
    std::istringstream iss(src);
    std::string tok;
    while(iss >> tok) {
        out.push_back({tok});
    }
    return out;
}

}
