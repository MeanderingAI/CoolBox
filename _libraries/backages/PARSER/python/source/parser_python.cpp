#include "headers/parser_python.h"
#include <stdexcept>

namespace ppython {
void Parser::parse() const {
    if(src.find("syntax_error")!=std::string::npos) {
        throw std::runtime_error("python: parse error");
    }
}
}
