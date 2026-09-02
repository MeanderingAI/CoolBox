#include "headers/parser_python.h"
#include <stdexcept>

namespace ppython {
void Parser::parse() const {
    if(src.find("syntax_error")!=std::string::npos) {
        std::string msg = std::string("python: parse error: ") + src;
        throw std::runtime_error(msg);
    }
}
}
