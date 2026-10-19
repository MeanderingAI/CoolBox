#include "../headers/quantum_simulator.h"

#include <cmath>

namespace quantum {
namespace gates {

Matrix2 identity() {
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}};
}

Matrix2 pauli_x() {
    return Matrix2{{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};
}

Matrix2 pauli_y() {
    return Matrix2{{0.0, 0.0}, {0.0, -1.0}, {0.0, 1.0}, {0.0, 0.0}};
}

Matrix2 pauli_z() {
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}};
}

Matrix2 hadamard() {
    const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    return Matrix2{{inv_sqrt2, 0.0}, {inv_sqrt2, 0.0}, {inv_sqrt2, 0.0}, {-inv_sqrt2, 0.0}};
}

Matrix2 phase_s() {
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 1.0}};
}

Matrix2 phase_s_dagger() {
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, -1.0}};
}

Matrix2 phase_t() {
    const double angle = M_PI / 4.0;
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {std::cos(angle), std::sin(angle)}};
}

Matrix2 phase_t_dagger() {
    const double angle = -M_PI / 4.0;
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {std::cos(angle), std::sin(angle)}};
}

Matrix2 rotation_x(double theta) {
    const double c = std::cos(theta / 2.0);
    const double s = std::sin(theta / 2.0);
    return Matrix2{{c, 0.0}, {0.0, -s}, {0.0, -s}, {c, 0.0}};
}

Matrix2 rotation_y(double theta) {
    const double c = std::cos(theta / 2.0);
    const double s = std::sin(theta / 2.0);
    return Matrix2{{c, 0.0}, {-s, 0.0}, {s, 0.0}, {c, 0.0}};
}

Matrix2 rotation_z(double theta) {
    const double half = theta / 2.0;
    return Matrix2{{std::cos(-half), std::sin(-half)}, {0.0, 0.0}, {0.0, 0.0}, {std::cos(half), std::sin(half)}};
}

Matrix2 phase(double theta) {
    return Matrix2{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {std::cos(theta), std::sin(theta)}};
}

} // namespace gates
} // namespace quantum
