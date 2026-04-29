#include "../headers/lsp_server.h"
#include <iostream>
#include <string>

int main() {
    std::string good = "function add(a,b)\n  c = a + b\nend\n";
    auto out_good = plang_process_text_for_diagnostics("file://good", good);
    if(out_good.find("diagnostics\":[]")==std::string::npos) {
        std::cerr<<"Expected no diagnostics for good input: "<<out_good<<"\n";
        return 1;
    }

    std::string bad = "function 123\n  x = ;\n"; // invalid
    auto out_bad = plang_process_text_for_diagnostics("file://bad", bad);
    if(out_bad.find("diagnostics\":[]")!=std::string::npos) {
        std::cerr<<"Expected diagnostics for bad input but got none\n";
        return 2;
    }

    std::cout<<"lsp tests passed\n";
    return 0;
}
