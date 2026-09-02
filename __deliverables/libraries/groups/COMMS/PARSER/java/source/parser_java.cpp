#include "headers/parser_java.h"
#include <stdexcept>

namespace pjava {

void Parser::parse() const {
    if(src.find("syntax_error")!=std::string::npos) {
        std::string msg = std::string("java: parse error: ") + src;
        throw std::runtime_error(msg);
    }
}

}
