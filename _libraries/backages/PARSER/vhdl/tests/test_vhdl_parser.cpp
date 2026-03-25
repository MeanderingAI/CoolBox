#include <iostream>
#include "vhdl/parser.h"

int main() {
    std::string src = "entity foo is\nend entity; architecture a of foo is signal x : std_logic; begin end architecture;";
    auto res = vhdl::parse_vhdl(src);
    std::cout << res << std::endl;
    if (res.find("entities=1") == std::string::npos) return 1;
    if (res.find("architectures=1") == std::string::npos) return 1;
    return 0;
}
