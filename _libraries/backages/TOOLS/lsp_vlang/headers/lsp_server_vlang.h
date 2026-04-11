#ifndef COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_VLANG_HEADERS_LSP_SERVER_VLANG_H
#define COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_VLANG_HEADERS_LSP_SERVER_VLANG_H

#include <string>

std::string vlang_process_text_for_diagnostics(const std::string& uri, const std::string& text);

#endif  // COOLBOX__LIBRARIES_BACKAGES_TOOLS_LSP_VLANG_HEADERS_LSP_SERVER_VLANG_H