#ifndef COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_MATLAB_HEADERS_LSP_SERVER_MATLAB_H
#define COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_MATLAB_HEADERS_LSP_SERVER_MATLAB_H
#include <string>

// Return a JSON params object for publishDiagnostics
std::string matlab_process_text_for_diagnostics(const std::string &uri, const std::string &text);

#endif  // COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_MATLAB_HEADERS_LSP_SERVER_MATLAB_H
