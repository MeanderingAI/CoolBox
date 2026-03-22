#include "headers/lsp_server_matlab.h"
#include <iostream>

int main(){
    std::string uri = "file://test.m";
    std::string sample = "a = 1;\nfunction y = f(x)\n y = x;\nend";
    auto out = matlab_process_text_for_diagnostics(uri, sample);
    std::cout << out << std::endl;
    return 0;
}
