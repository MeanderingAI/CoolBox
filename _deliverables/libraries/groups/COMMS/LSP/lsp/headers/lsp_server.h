#ifndef COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_HEADERS_LSP_SERVER_H
#define COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_HEADERS_LSP_SERVER_H
#include <string>

// Process document text and return a JSON string representing a
// `textDocument/publishDiagnostics` params object (not full LSP envelope).
std::string plang_process_text_for_diagnostics(const std::string &uri, const std::string &text);

#endif  // COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_HEADERS_LSP_SERVER_H
