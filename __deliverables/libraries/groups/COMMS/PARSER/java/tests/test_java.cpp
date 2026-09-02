#include "parser_java.h"
#include <iostream>

int main(){
    pjava::Parser p("class A {};");
    try{ p.parse(); }catch(...){ std::cerr<<"unexpected"<<std::endl; return 1; }
    pjava::Parser q("syntax_error");
    try{ q.parse(); std::cerr<<"expected fail"<<std::endl; return 2; }catch(...){}
    return 0;
}
