#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include "lsp_server_rust.h"

int main(int argc, char **argv) {
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
            std::string params = rust_process_text_for_diagnostics(uri, text);
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
