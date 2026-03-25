#include "headers/parser_rust.h"
#include <iostream>

int main(){
    const char *good = "fn main() { let x = 1; }";
    prust::Parser p(good);
    try{
        p.parse();
    }catch(...){
        std::cerr << "unexpected parse failure" << std::endl;
        return 1;
    }
    const char *bad = "syntax_error";
    prust::Parser q(bad);
    try{
        q.parse();
        std::cerr << "expected failure" << std::endl;
        return 2;
    }catch(...){
        // expected
    }
    return 0;
}
