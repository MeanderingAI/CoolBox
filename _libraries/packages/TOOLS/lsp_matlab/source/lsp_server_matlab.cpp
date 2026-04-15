#include "lsp_server_matlab.h"
#include <sstream>
#include <exception>
#include <matlab/parser.h>

std::string matlab_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    try {
        std::string summary = matlab::parse_matlab(text);
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
