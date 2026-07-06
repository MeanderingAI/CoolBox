#include "../headers/lsp_server.h"
#include <sstream>
#include <exception>
#include "lexer.h"
#include "parser.h"

using namespace plang;

std::string plang_process_text_for_diagnostics(const std::string &uri, const std::string &text) {
    // Try to lex and parse; on error return a publishDiagnostics params JSON
    try {
        Lexer lx(text);
        auto toks = lx.tokenize();
        Parser p(toks);
        auto prog = p.parse();
        // no diagnostics
        std::ostringstream ok;
        ok << "{\"uri\":\"" << uri << "\",\"diagnostics\":[]}";
        return ok.str();
    } catch(const std::exception &ex) {
        // Return a single diagnostic at line 0, char 0 with the exception message
        std::ostringstream out;
        out << "{\"uri\":\"" << uri << "\",\"diagnostics\":[{";
        out << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":1}},";
        out << "\"severity\":1,\"message\":\"";
        // escape quotes in message
        std::string msg = ex.what();
        for(char c: msg) {
            if(c=='\"') out << "\\\"";
            else if(c=='\\') out << "\\\\";
            else out << c;
        }
        out << "\"}]}";
        return out.str();
    }
}
