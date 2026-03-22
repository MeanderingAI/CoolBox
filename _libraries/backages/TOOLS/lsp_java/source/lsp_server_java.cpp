#include "headers/lsp_server_java.h"
#include <sstream>
#include <exception>
#include "../../PARSER/java/headers/parser_java.h"

std::string java_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    try {
        pjava::Parser p(text);
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
