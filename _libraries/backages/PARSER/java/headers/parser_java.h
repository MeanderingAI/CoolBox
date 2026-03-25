#pragma once
#include <string>

namespace pjava {
class Parser {
public:
    explicit Parser(const std::string &s): src(s) {}
    void parse() const;
private:
    std::string src;
};
}
