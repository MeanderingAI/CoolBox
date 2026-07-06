#ifndef COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_VHDL_HEADERS_LSP_SERVER_VHDL_H
#define COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_VHDL_HEADERS_LSP_SERVER_VHDL_H
#include <string>

// Return a JSON params object for publishDiagnostics
std::string vhdl_process_text_for_diagnostics(const std::string &uri, const std::string &text);

#endif  // COOLBOX__LIBRARIES_PACKAGES_TOOLS_LSP_VHDL_HEADERS_LSP_SERVER_VHDL_H
