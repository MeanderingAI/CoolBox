#include "../headers/lexer.h"
#include "../headers/parser.h"
#include <iostream>

int main() {
    using namespace plang;
    const std::string src =
        "function add(a,b)\n"
        "  c = a + b\n"
        "end\n";

    Lexer lx(src);
    auto toks = lx.tokenize();
    Parser p(toks);
    auto prog = p.parse();
    prog->dump();
    return 0;
}
