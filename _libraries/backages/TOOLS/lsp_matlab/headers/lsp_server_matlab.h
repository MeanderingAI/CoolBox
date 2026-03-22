#pragma once
#include <string>

// Return a JSON params object for publishDiagnostics
std::string matlab_process_text_for_diagnostics(const std::string &uri, const std::string &text);
