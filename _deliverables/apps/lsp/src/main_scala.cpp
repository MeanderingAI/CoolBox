#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "lsp_server_scala.h"

int main(int argc, char** argv) {
    std::cin.sync_with_stdio(false);
    std::cout.sync_with_stdio(false);
    std::string line;
    while (true) {
        std::map<std::string, std::string> headers;
        while (true) {
            if (!std::getline(std::cin, line)) {
                return 0;
            }
            if (line.empty() || line == "\r") {
                break;
            }
            const auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                    value.erase(0, 1);
                }
                if (!value.empty() && value.back() == '\r') {
                    value.pop_back();
                }
                headers[key] = value;
            }
        }
        const auto length_it = headers.find("Content-Length");
        if (length_it == headers.end()) {
            continue;
        }
        const size_t length = std::stoul(length_it->second);
        std::string body(length, '\0');
        std::cin.read(&body[0], length);

        if (body.find("textDocument/didOpen") != std::string::npos || body.find("textDocument/didChange") != std::string::npos) {
            const auto find_field = [&](const std::string& field) {
                const auto field_pos = body.find("\"" + field + "\"");
                if (field_pos == std::string::npos) {
                    return std::string();
                }
                const auto colon_pos = body.find(':', field_pos);
                if (colon_pos == std::string::npos) {
                    return std::string();
                }
                const auto start_quote = body.find_first_of('"', colon_pos);
                if (start_quote == std::string::npos) {
                    return std::string();
                }
                const auto end_quote = body.find_first_of('"', start_quote + 1);
                if (end_quote == std::string::npos) {
                    return std::string();
                }
                return body.substr(start_quote + 1, end_quote - start_quote - 1);
            };

            std::string uri = find_field("uri");
            std::string text = find_field("text");
            if (uri.empty()) {
                uri = "file://stdin";
            }

            const std::string params = scala_process_text_for_diagnostics(uri, text);
            std::ostringstream diag;
            diag << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":" << params << "}";
            std::ostringstream out;
            out << "Content-Length: " << diag.str().size() << "\r\n\r\n" << diag.str();
            std::cout << out.str();
            std::cout.flush();
        }
        if (body.find("\"method\":\"shutdown\"") != std::string::npos) {
            break;
        }
    }
    return 0;
}
