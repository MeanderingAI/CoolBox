#include "lsp_server_scala.h"

#include <iostream>
#include <string>

int main() {
    const auto ok = scala_process_text_for_diagnostics(
        "file://Main.scala",
        "object Main { def main(args: Array[String]): Unit = { println(\"ok\") } }");
    const auto bad = scala_process_text_for_diagnostics(
        "file://Main.scala",
        "object Main { def main(args: Array[String]): Unit = { syntax_error ");
    if (ok.find("\"diagnostics\":[]") == std::string::npos) {
        std::cerr << "expected empty diagnostics" << std::endl;
        return 1;
    }
    if (bad.find("syntax error") == std::string::npos) {
        std::cerr << "expected syntax error diagnostic" << std::endl;
        return 2;
    }
    return 0;
}
