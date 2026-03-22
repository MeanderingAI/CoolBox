#include "headers/parser_java.h"
#include <stdexcept>

namespace pjava {

void Parser::parse() const {
    if(src.find("syntax_error")!=std::string::npos) {
        throw std::runtime_error("java: parse error");
    }
}

}
