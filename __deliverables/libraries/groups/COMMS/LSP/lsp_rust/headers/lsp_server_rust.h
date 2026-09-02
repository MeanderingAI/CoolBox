#ifndef COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_RUST_HEADERS_LSP_SERVER_RUST_H
#define COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_RUST_HEADERS_LSP_SERVER_RUST_H
#include <string>

// Return a JSON params object for publishDiagnostics
std::string rust_process_text_for_diagnostics(const std::string &uri, const std::string &text);

#endif  // COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_RUST_HEADERS_LSP_SERVER_RUST_H
