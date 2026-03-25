#include <iostream>
#include "matlab/parser.h"

int main() {
    std::string src = "function y = f(x)\nfor i=1:10\nend\nend";
    auto res = matlab::parse_matlab(src);
    std::cout << res << std::endl;
    if (res.find("functions=1") == std::string::npos) return 1;
    if (res.find("fors=1") == std::string::npos) return 1;
    return 0;
}
