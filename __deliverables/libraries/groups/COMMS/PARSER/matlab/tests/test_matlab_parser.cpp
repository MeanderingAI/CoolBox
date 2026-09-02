#include <iostream>
#include <cctype>
#include "matlab/parser.h"

static int extract_count(const std::string &s, const std::string &key) {
    auto pos = s.find(key);
    if (pos == std::string::npos) return -1;
    pos += key.size();
    size_t end = pos;
    while (end < s.size() && std::isdigit(static_cast<unsigned char>(s[end]))) ++end;
    try {
        return std::stoi(s.substr(pos, end - pos));
    } catch(...) { return -1; }
}

int main() {
    std::string src = "function y = f(x)\nfor i=1:10\nend\nend";
    auto res = matlab::parse_matlab(src);
    std::cout << res << std::endl;
    int funcs = extract_count(res, "functions=");
    int fors = extract_count(res, "fors=");
    if (funcs != 1) return 1;
    if (fors != 1) return 1;
    return 0;
}
