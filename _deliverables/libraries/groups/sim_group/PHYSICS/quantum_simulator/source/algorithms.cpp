#include "../headers/quantum_algorithms.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace quantum {
namespace algorithms {

QuantumCircuit bell_state() {
    QuantumCircuit circuit(2);
    circuit.h(0).cnot(0, 1);
    return circuit;
}

QuantumCircuit ghz_state(int num_qubits) {
    if (num_qubits < 2) throw std::invalid_argument("ghz_state: num_qubits must be >= 2");
    QuantumCircuit circuit(num_qubits);
    circuit.h(0);
    for (int q = 1; q < num_qubits; ++q) circuit.cnot(0, q);
    return circuit;
}

QuantumState deutsch_jozsa_state(int num_input_qubits, const BooleanOracle& oracle) {
    if (num_input_qubits < 1) throw std::invalid_argument("deutsch_jozsa_state: num_input_qubits must be >= 1");

    const int ancilla = num_input_qubits;
    QuantumState state(num_input_qubits + 1);

    // |0...0>|0> -> |0...0>|1> -> H^(n+1) -> uniform superposition x (|0>-|1>)/sqrt(2)
    state.apply_single_qubit_gate(ancilla, gates::pauli_x());
    for (int q = 0; q <= ancilla; ++q) state.apply_single_qubit_gate(q, gates::hadamard());

    // Oracle U_f: |x>|y> -> |x>|y XOR f(x)>. This is a bijection on the full
    // state (for a fixed x, it's identity when f(x)=0 and a bit-flip on the
    // ancilla when f(x)=1), so it's applied directly as a permutation -- see
    // quantum_algorithms.h's oracle note.
    const std::uint64_t input_mask = (std::uint64_t{1} << num_input_qubits) - 1;
    const std::uint64_t ancilla_mask = std::uint64_t{1} << ancilla;
    state.apply_permutation([&](std::uint64_t i) -> std::uint64_t {
        const std::uint64_t x = i & input_mask;
        if (oracle(x) & 1) return i ^ ancilla_mask;
        return i;
    });

    // Final Hadamards on the input register only (ancilla is left alone).
    for (int q = 0; q < num_input_qubits; ++q) state.apply_single_qubit_gate(q, gates::hadamard());
    return state;
}

bool deutsch_jozsa(int num_input_qubits, const BooleanOracle& oracle, std::mt19937_64& rng) {
    QuantumState state = deutsch_jozsa_state(num_input_qubits, oracle);
    std::uint64_t result = 0;
    for (int q = 0; q < num_input_qubits; ++q) {
        const int bit = state.measure_qubit(q, rng);
        result |= (static_cast<std::uint64_t>(bit) << q);
    }
    return result == 0; // all-zero => constant; anything else => balanced
}

int optimal_grover_iterations(int num_qubits) {
    const double n = static_cast<double>(std::uint64_t{1} << num_qubits);
    const int iterations = static_cast<int>(std::floor((M_PI / 4.0) * std::sqrt(n)));
    return std::max(1, iterations);
}

QuantumState grover_state(int num_qubits, std::uint64_t marked_index) {
    if (num_qubits < 1) throw std::invalid_argument("grover_state: num_qubits must be >= 1");
    const std::uint64_t n = std::uint64_t{1} << num_qubits;
    if (marked_index >= n) throw std::invalid_argument("grover_state: marked_index out of range");

    QuantumState state(num_qubits);
    for (int q = 0; q < num_qubits; ++q) state.apply_single_qubit_gate(q, gates::hadamard());

    const int iterations = optimal_grover_iterations(num_qubits);
    for (int iter = 0; iter < iterations; ++iter) {
        // Oracle: flip the sign of the marked amplitude (a diagonal phase of
        // pi, i.e. multiply by e^{i*pi} = -1).
        state.apply_diagonal_phase([&](std::uint64_t i) {
            return (i == marked_index) ? M_PI : 0.0;
        });

        // Diffusion operator D = H^n (2|0><0| - I) H^n ("inversion about the
        // mean"): Hadamard everything, flip the phase of every amplitude
        // except |0...0>, then Hadamard everything again.
        for (int q = 0; q < num_qubits; ++q) state.apply_single_qubit_gate(q, gates::hadamard());
        state.apply_diagonal_phase([&](std::uint64_t i) {
            return (i == 0) ? 0.0 : M_PI;
        });
        for (int q = 0; q < num_qubits; ++q) state.apply_single_qubit_gate(q, gates::hadamard());
    }
    return state;
}

std::uint64_t grover_search(int num_qubits, std::uint64_t marked_index, std::mt19937_64& rng) {
    QuantumState state = grover_state(num_qubits, marked_index);
    return state.measure_all(rng);
}

} // namespace algorithms
} // namespace quantum
