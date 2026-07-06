#include "headers/lsp_server_rust.h"
#include <string>
#include <sstream>
#include <exception>
#include "../../../PARSER/rust/headers/parser_rust.h"

std::string rust_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    try {
        prust::Parser p(text);
        p.parse();
        std::ostringstream o;
        o << "{\"uri\":\"" << uri << "\",\"diagnostics\":[]}";
        return o.str();
    } catch(const std::exception &e) {
        std::ostringstream o;
        o << "{\"uri\":\"" << uri << "\",\"diagnostics\":[{\"message\":\"" << e.what() << "\",\"severity\":1}]}";
        return o.str();
    }
}
