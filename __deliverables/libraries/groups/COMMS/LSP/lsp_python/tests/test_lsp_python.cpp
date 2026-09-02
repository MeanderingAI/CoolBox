#include "headers/lsp_server_python.h"
#include <iostream>

int main(){
    auto ok = python_process_text_for_diagnostics("file://x","def f(): pass");
    auto bad = python_process_text_for_diagnostics("file://x","syntax_error");
    if(ok.find("diagnostics")==std::string::npos) { std::cerr<<"no diagnostics field"<<std::endl; return 1; }
    if(bad.find("syntax_error")==std::string::npos) { std::cerr<<"expected message"<<std::endl; return 2; }
    return 0;
}
