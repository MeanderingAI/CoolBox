#include "lsp_server_vlang.h"

#include <iostream>

int main() {
    const auto ok = vlang_process_text_for_diagnostics("file://main.v", "module main\nfn main() { println('ok') }");
    const auto bad = vlang_process_text_for_diagnostics("file://main.v", "fn main() { syntax_error ");
    if (ok.find("\"diagnostics\":[]") == std::string::npos) {
        std::cerr << "expected empty diagnostics" << std::endl;
        return 1;
    }
    if (bad.find("syntax error") == std::string::npos) {
        std::cerr << "expected syntax error diagnostic" << std::endl;
        return 2;
    }
    return 0;
}