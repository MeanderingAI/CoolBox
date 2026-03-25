#include "headers/parser_python.h"
#include <iostream>

int main(){
    ppython::Parser p("def f(): pass");
    try{ p.parse(); }catch(...){ std::cerr<<"unexpected"<<std::endl; return 1; }
    ppython::Parser q("syntax_error");
    try{ q.parse(); std::cerr<<"expected fail"<<std::endl; return 2; }catch(...){}
    return 0;
}
