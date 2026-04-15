#include "headers/lexer_java.h"
#include <sstream>

namespace pjava {

std::vector<Token> Lexer::tokenize() const {
    std::vector<Token> out;
    std::istringstream iss(src);
    std::string t;
    while(iss >> t) out.push_back({t});
    return out;
}

}
