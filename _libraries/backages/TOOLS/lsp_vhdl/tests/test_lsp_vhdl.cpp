#include "headers/lsp_server_vhdl.h"
#include <iostream>

int main(){
    std::string uri = "file://test.vhd";
    std::string sample = "entity foo is\nend entity;";
    auto out = vhdl_process_text_for_diagnostics(uri, sample);
    std::cout << out << std::endl;
    return 0;
}
