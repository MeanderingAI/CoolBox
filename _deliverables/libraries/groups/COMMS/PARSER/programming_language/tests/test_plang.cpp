#include "../headers/lexer.h"
#include "../headers/parser.h"
#include <iostream>
#include <memory>

using namespace plang;

int main() {
    const std::string src =
        "function add(a,b)\n"
        "  c = a + b\n"
        "end\n";

    Lexer lx(src);
    auto toks = lx.tokenize();
    // basic checks
    bool found_function=false;
    for(auto &t: toks) {
        if(t.type==TokenType::Keyword && t.text=="function") { found_function=true; break; }
    }
    if(!found_function) { std::cerr<<"Lexer failed to find 'function' keyword\n"; return 1; }

    Parser p(toks);
    auto prog = p.parse();
    if(!prog) { std::cerr<<"Parser returned null program\n"; return 2; }
    // find first function decl
    bool ok=false;
    for(auto &it: prog->items) {
        if(auto fd = dynamic_cast<FunctionDecl*>(it.get())) {
            if(fd->name=="add") { ok=true; break; }
        }
    }
    if(!ok) { std::cerr<<"Did not find function 'add' in AST\n"; return 3; }
    std::cout<<"plang test passed\n";
    return 0;
}
