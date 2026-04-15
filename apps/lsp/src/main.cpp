#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include "lsp_server.h"
#include "../../_libraries/packages/TOOLS/lsp_rust/headers/lsp_server_rust.h"
#include "../../_libraries/packages/TOOLS/lsp_java/headers/lsp_server_java.h"
#include "../../_libraries/packages/TOOLS/lsp_python/headers/lsp_server_python.h"
// Add VHDL & MATLAB LSP handlers
#include "../../_libraries/packages/TOOLS/lsp_vhdl/headers/lsp_server_vhdl.h"
#include "../../_libraries/packages/TOOLS/lsp_matlab/headers/lsp_server_matlab.h"

// Unified LSP server main. Accepts --lang=<plang|rust|java|python|vhdl|matlab> (default: plang)
enum class Lang { PLANG, RUST, JAVA, PYTHON, VHDL, MATLAB };

static Lang parse_lang_arg(int argc, char** argv){
    std::string def = "plang";
    for(int i=1;i<argc;++i){
        std::string s = argv[i];
        auto pos = s.find("--lang=");
            if(pos!=std::string::npos){
            std::string v = s.substr(pos+7);
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            if(v=="rust") return Lang::RUST;
            if(v=="java") return Lang::JAVA;
            if(v=="python") return Lang::PYTHON;
            if(v=="vhdl") return Lang::VHDL;
            if(v=="matlab" || v=="m") return Lang::MATLAB;
            return Lang::PLANG;
        }
    }
    return Lang::PLANG;
}

int main(int argc, char **argv) {
    Lang lang = parse_lang_arg(argc, argv);

    std::cin.sync_with_stdio(false);
    std::cout.sync_with_stdio(false);

    std::string line;
    while(true) {
        std::map<std::string,std::string> hdr;
        while(true) {
            if(!std::getline(std::cin, line)) return 0;
            if(line.empty() || line=="\r") break;
            auto pos = line.find(':');
            if(pos!=std::string::npos) {
                std::string k = line.substr(0,pos);
                std::string v = line.substr(pos+1);
                while(!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
                if(!v.empty() && v.back()=='\r') v.pop_back();
                hdr[k]=v;
            }
        }
        auto it = hdr.find("Content-Length");
        if(it==hdr.end()) continue;
        size_t len = std::stoul(it->second);
        std::string body;
        body.resize(len);
        std::cin.read(&body[0], len);

        if(body.find("textDocument/didOpen")!=std::string::npos || body.find("textDocument/didChange")!=std::string::npos) {
            auto find_field = [&](const std::string &field){
                auto p = body.find("\"" + field + "\"");
                if(p==std::string::npos) return std::string();
                auto colon = body.find(':', p);
                if(colon==std::string::npos) return std::string();
                auto q = body.find_first_of('"', colon);
                if(q==std::string::npos) return std::string();
                auto r = body.find_first_of('"', q+1);
                if(r==std::string::npos) return std::string();
                return body.substr(q+1, r-q-1);
            };
            std::string uri = find_field("uri");
            std::string text = find_field("text");
            if(uri.empty()) uri = "file://stdin";

            std::string params;
            switch(lang){
                case Lang::PLANG: params = plang_process_text_for_diagnostics(uri, text); break;
                case Lang::RUST: params = rust_process_text_for_diagnostics(uri, text); break;
                case Lang::JAVA: params = java_process_text_for_diagnostics(uri, text); break;
                case Lang::PYTHON: params = python_process_text_for_diagnostics(uri, text); break;
                case Lang::VHDL: params = vhdl_process_text_for_diagnostics(uri, text); break;
                case Lang::MATLAB: params = matlab_process_text_for_diagnostics(uri, text); break;
            }

            std::ostringstream diag;
            diag << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":" << params << "}";
            std::ostringstream out;
            out << "Content-Length: " << diag.str().size() << "\r\n\r\n" << diag.str();
            std::cout << out.str();
            std::cout.flush();
        }
        if(body.find("\"method\":\"shutdown\"")!=std::string::npos) break;
    }
    return 0;
}
