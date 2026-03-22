#pragma once
#include <string>

namespace prust {

class Parser {
public:
    explicit Parser(const std::string &src): src(src) {}
    // Throws std::runtime_error on parse error
    void parse() const;
private:
    std::string src;
};

}
