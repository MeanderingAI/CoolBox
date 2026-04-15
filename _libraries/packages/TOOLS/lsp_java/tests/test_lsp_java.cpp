#include "headers/lsp_server_java.h"
#include <iostream>

int main(){
    auto ok = java_process_text_for_diagnostics("file://x","class A{};");
    auto bad = java_process_text_for_diagnostics("file://x","syntax_error");
    if(ok.find("diagnostics")==std::string::npos) { std::cerr<<"no diagnostics field"<<std::endl; return 1; }
    if(bad.find("syntax_error")==std::string::npos) { std::cerr<<"expected message"<<std::endl; return 2; }
    return 0;
}
