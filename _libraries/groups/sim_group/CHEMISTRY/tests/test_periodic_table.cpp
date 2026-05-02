#include <iostream>
#include "chemistry/periodic_table.h"

int main() {
    using chemistry::element_by_symbol;
    using chemistry::element_by_atomic_number;

    bool ok = true;

    auto li = element_by_symbol("Li");
    if (li.atomic_number != 3) {
        std::cerr << "Li atomic number expected 3, got " << li.atomic_number << "\n";
        ok = false;
    }

    auto fe = element_by_symbol("Fe");
    if (fe.atomic_number != 26) {
        std::cerr << "Fe atomic number expected 26, got " << fe.atomic_number << "\n";
        ok = false;
    }

    auto o = element_by_atomic_number(8);
    if (std::string(o.symbol) != "O") {
        std::cerr << "Atomic number 8 expected symbol O, got " << o.symbol << "\n";
        ok = false;
    }

    auto unk = element_by_symbol("Xx");
    if (unk.atomic_number != 0) {
        std::cerr << "Unknown symbol should return atomic_number 0\n";
        ok = false;
    }

    if (!ok) return 1;
    std::cout << "periodic_table tests passed" << std::endl;
    return 0;
}
