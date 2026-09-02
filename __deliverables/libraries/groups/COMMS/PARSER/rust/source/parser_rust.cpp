#include "headers/parser_rust.h"
#include <stdexcept>
#include <string>

namespace prust {

void Parser::parse() const {
    if(src.find("syntax_error")!=std::string::npos) {
        throw std::runtime_error("rust: parse error: found 'syntax_error'");
    }
    // minimal: otherwise accept
}

}
