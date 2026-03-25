#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

// Very small LSP-ish stdio server stub. It reads Content-Length framed JSON
// messages and, on receiving textDocument/didOpen or didChange, publishes
// a placeholder diagnostic back to the client.

static bool read_headers(std::istream &in, std::map<std::string,std::string> &hdr) {
    std::string line;
    while(true) {
        if(!std::getline(in, line)) return false;
        if(line.empty() || line=="\r") break;
        auto pos = line.find(':');
        if(pos!=std::string::npos) {
            std::string k = line.substr(0,pos);
            std::string v = line.substr(pos+1);
            // trim
            while(!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
            if(!v.empty() && v.back()=='\r') v.pop_back();
            hdr[k]=v;
        }
    }
    return true;
}

static std::string read_body(std::istream &in, size_t len) {
    std::string body;
    body.resize(len);
    in.read(&body[0], len);
    return body;
}

static void send_message(const std::string &payload) {
    std::ostringstream out;
    out << "Content-Length: " << payload.size() << "\r\n\r\n" << payload;
    std::string s = out.str();
    std::cout << s;
    std::cout.flush();
}

// naive extractor helpers
static std::string extract_field(const std::string &json, const std::string &field) {
    auto p = json.find("\"" + field + "\"");
    if(p==std::string::npos) return {};
    auto colon = json.find(':', p);
    if(colon==std::string::npos) return {};
    auto q = json.find_first_of('"', colon);
    if(q==std::string::npos) return {};
    auto r = json.find_first_of('"', q+1);
    if(r==std::string::npos) return {};
    return json.substr(q+1, r-q-1);
}

int main() {
    std::cin.sync_with_stdio(false);
    std::cout.sync_with_stdio(false);

    while(true) {
        std::map<std::string,std::string> hdr;
        if(!read_headers(std::cin, hdr)) break;
        auto it = hdr.find("Content-Length");
        if(it==hdr.end()) continue;
        size_t len = std::stoul(it->second);
        std::string body = read_body(std::cin, len);

        // naive detection of didOpen/didChange
        if(body.find("textDocument/didOpen")!=std::string::npos || body.find("textDocument/didChange")!=std::string::npos) {
            std::string uri = extract_field(body, "uri");
            if(uri.empty()) uri = "file://stdin";

            // try to extract the document text (didOpen has text, didChange uses contentChanges[0].text)
            std::string text;
            auto ptext = body.find("\"text\"");
            if(ptext!=std::string::npos) {
                // crude extraction: find the first occurrence of "text" after uri and take the following string value
                text = extract_field(body, "text");
            }

            if(text.empty()) text = "";

            // call helper to compute diagnostics using plang
            std::string params = plang_process_text_for_diagnostics(uri, text);
            std::ostringstream diag;
            diag << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":" << params << "}";
            send_message(diag.str());
        }

        // handle shutdown request
        if(body.find("\"method\":\"shutdown\"")!=std::string::npos) break;
    }

    return 0;
}
