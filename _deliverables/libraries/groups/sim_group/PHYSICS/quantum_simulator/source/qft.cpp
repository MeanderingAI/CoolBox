#include "../headers/quantum_simulator.h"

#include <cmath>

namespace quantum {

// Standard QFT decomposition (Nielsen & Chuang Fig. 5.1), adapted to
// `qubits` given least-significant-qubit-first (qubits[0] = bit 0 of the
// register's integer value, ..., qubits[n-1] = the MSB):
//
//   for idx = n-1 downto 0:              // process the MSB first
//       H(qubits[idx])
//       for idx2 = idx-1 downto 0:       // every less-significant qubit
//           controlled-phase(control=qubits[idx2], target=qubits[idx],
//                             angle = 2*pi / 2^(idx-idx2+1))
//   then reverse the qubit order (swap p <-> n-1-p for p < n/2), since the
//   cascade above naturally produces bit-reversed output.
//
// (Processing order matters here: each target qubit's cascade of
// controlled-phase gates must run using controls that haven't themselves
// been Hadamard'd yet, matching the textbook circuit exactly — an earlier
// draft of this function processed qubits LSB-first instead of MSB-first,
// which silently produced *a* valid-looking unitary but the wrong one;
// verified against the closed-form QFT matrix via unit tests.)
//
// The inverse QFT is the adjoint: run the reversal first, then the same
// cascade in the opposite order with negated angles.
void apply_qft(QuantumState& state, const std::vector<int>& qubits, bool inverse) {
    const int n = static_cast<int>(qubits.size());
    if (n == 0) return;

    auto angle_for = [](int idx, int idx2) {
        return 2.0 * M_PI / static_cast<double>(std::uint64_t{1} << (idx - idx2 + 1));
    };

    if (!inverse) {
        for (int idx = n - 1; idx >= 0; --idx) {
            state.apply_single_qubit_gate(qubits[idx], gates::hadamard());
            for (int idx2 = idx - 1; idx2 >= 0; --idx2) {
                state.apply_controlled_gate(qubits[idx2], qubits[idx], gates::phase(angle_for(idx, idx2)));
            }
        }
        for (int p = 0; p < n / 2; ++p) {
            state.apply_swap(qubits[p], qubits[n - 1 - p]);
        }
    } else {
        for (int p = 0; p < n / 2; ++p) {
            state.apply_swap(qubits[p], qubits[n - 1 - p]);
        }
        for (int idx = 0; idx < n; ++idx) {
            for (int idx2 = 0; idx2 < idx; ++idx2) {
                state.apply_controlled_gate(qubits[idx2], qubits[idx], gates::phase(-angle_for(idx, idx2)));
            }
            state.apply_single_qubit_gate(qubits[idx], gates::hadamard());
        }
    }
}

} // namespace quantum
