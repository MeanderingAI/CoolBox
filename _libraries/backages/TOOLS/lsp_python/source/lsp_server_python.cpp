#include "headers/lsp_server_python.h"
#include <sstream>
#include <exception>
#include "../../PARSER/python/headers/parser_python.h"

std::string python_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    try {
        ppython::Parser p(text);
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
