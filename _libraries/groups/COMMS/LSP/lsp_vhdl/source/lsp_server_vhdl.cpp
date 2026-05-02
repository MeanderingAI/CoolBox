#include "lsp_server_vhdl.h"
#include <sstream>
#include <exception>
#include <vhdl/parser.h>

std::string vhdl_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    try {
        std::string summary = vhdl::parse_vhdl(text);
        std::ostringstream ok;
        ok << "{\"uri\":\"" << uri << "\",\"diagnostics\":[]}";
        return ok.str();
    } catch(const std::exception &ex) {
        std::ostringstream out;
        out << "{\"uri\":\"" << uri << "\",\"diagnostics\":[{";
        out << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":1}},";
        out << "\"severity\":1,\"message\":\"";
        std::string msg = ex.what();
        for(char c: msg) {
            if(c=='"') out << "\\\"";
            else if(c=='\\') out << "\\\\";
            else out << c;
        }
        out << "\"}]}";
        return out.str();
    }
}
