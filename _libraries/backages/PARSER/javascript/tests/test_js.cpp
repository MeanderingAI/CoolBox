#include "../headers/lexer_js.h"
#include "../headers/parser_js.h"
#include <iostream>

int main() {
    using namespace pljs;
    const std::string good =
        "function add(a,b) {\n"
        "  return a + b;\n"
        "}\n";

    LexerJS lx(good);
    auto toks = lx.tokenize();
    ParserJS p(toks);
    try {
        if(!p.parse()) { std::cerr<<"Parser returned false for good input\n"; return 1; }
    } catch(const std::exception &ex) { std::cerr<<"Parser threw for good input: "<<ex.what()<<"\n"; return 2; }

    const std::string bad = "function 123 { x = ; }\n";
    LexerJS lx2(bad);
    auto toks2 = lx2.tokenize();
    ParserJS p2(toks2);
    try {
        p2.parse();
        std::cerr<<"Expected parser to fail for bad input\n"; return 3;
    } catch(...) {
        // expected
    }

    std::cout<<"js parser tests passed\n";
    return 0;
}
