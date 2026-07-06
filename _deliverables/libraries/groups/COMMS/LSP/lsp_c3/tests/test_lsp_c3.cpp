#include "lsp_server_c3.h"

#include <iostream>

int main() {
    const auto ok = c3_process_text_for_diagnostics("file://main.c3", "fn void main() { int x = 1; }");
    const auto bad = c3_process_text_for_diagnostics("file://main.c3", "fn void main( { syntax_error");
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