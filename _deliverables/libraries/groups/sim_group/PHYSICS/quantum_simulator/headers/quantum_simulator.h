#ifndef COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_SIMULATOR_H
#define COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_SIMULATOR_H

// quantum_simulator — an exact (noiseless), state-vector quantum computer
// simulator: dense complex amplitudes for every computational basis state,
// standard single-qubit gates, controlled/multi-controlled gates, SWAP,
// measurement (single-qubit and full), and a QuantumCircuit builder that
// records a gate sequence for reuse/inspection.
//
// This is a *simulator*, not a real quantum computer: it keeps 2^num_qubits
// complex<double> amplitudes in memory, so it scales exponentially and is
// only practical up to roughly 20-24 qubits (2^24 * 16 bytes = 256 MiB).
// That's still plenty for teaching/demo purposes (Bell/GHZ states,
// Deutsch-Jozsa, Grover's search, Shor's algorithm on small numbers — see
// quantum_algorithms.h).
//
// Qubit indexing is little-endian: qubit 0 is the least-significant bit of
// the basis-state index, i.e. basis state |q_{n-1} ... q_1 q_0> corresponds
// to amplitude index (q_{n-1}<<(n-1)) | ... | (q_1<<1) | q_0.

#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace quantum {

using Complex = std::complex<double>;

// A single-qubit unitary, row-major: [[m00, m01], [m10, m11]].
struct Matrix2 {
    Complex m00{1.0, 0.0};
    Complex m01{0.0, 0.0};
    Complex m10{0.0, 0.0};
    Complex m11{1.0, 0.0};
};

namespace gates {
    Matrix2 identity();
    Matrix2 pauli_x();   // X / NOT
    Matrix2 pauli_y();   // Y
    Matrix2 pauli_z();   // Z
    Matrix2 hadamard();  // H
    Matrix2 phase_s();        // S = diag(1, i)
    Matrix2 phase_s_dagger();
    Matrix2 phase_t();        // T = diag(1, e^{i*pi/4})
    Matrix2 phase_t_dagger();
    Matrix2 rotation_x(double theta);
    Matrix2 rotation_y(double theta);
    Matrix2 rotation_z(double theta);
    Matrix2 phase(double theta); // diag(1, e^{i*theta})
} // namespace gates

class QuantumState {
public:
    // Maximum qubit count this simulator will allocate for (2^28 complex
    // doubles would be 4 GiB; refuse before anyone accidentally OOMs).
    static constexpr int kMaxQubits = 28;

    explicit QuantumState(int num_qubits);

    int num_qubits() const { return num_qubits_; }
    std::size_t dimension() const { return amplitudes_.size(); }
    const std::vector<Complex>& amplitudes() const { return amplitudes_; }

    // Applies a single-qubit unitary to `qubit` (equivalent to
    // apply_multi_controlled_gate({}, qubit, gate)).
    void apply_single_qubit_gate(int qubit, const Matrix2& gate);

    // Applies `gate` to `target`, but only within the subspace where
    // `control` is |1> (the rest of the state is left untouched). Pass
    // gates::pauli_x() for a standard CNOT, gates::pauli_z() for CZ, etc.
    void apply_controlled_gate(int control, int target, const Matrix2& gate);

    // Generalized controlled gate: `gate` is applied to `target` only
    // within the subspace where every qubit in `controls` is |1>. An empty
    // `controls` list makes this an unconditional single-qubit gate; a
    // single control makes it a standard controlled gate; two controls
    // gives a Toffoli/CCX-style gate (pass gates::pauli_x()).
    void apply_multi_controlled_gate(const std::vector<int>& controls, int target, const Matrix2& gate);

    // Swaps the amplitudes of `qubit_a` and `qubit_b`.
    void apply_swap(int qubit_a, int qubit_b);

    // Applies an arbitrary permutation of the computational basis:
    // |i> -> |permute(i)>. Any bijection on [0, dimension()) is a valid
    // unitary (it's just a relabelling of orthonormal basis vectors), so
    // this is how classically-described reversible oracles (XOR oracles,
    // modular multiplication, etc.) are simulated directly, instead of
    // hand-compiling a gate-level reversible arithmetic circuit (adders,
    // carry chains, ...). The *result* is mathematically identical to what
    // such a circuit would compute; only the implementation shortcut is
    // classical. `permute` must be a true bijection on [0, dimension()) —
    // this is the caller's responsibility and is not checked here.
    void apply_permutation(const std::function<std::uint64_t(std::uint64_t)>& permute);

    // Applies an arbitrary diagonal unitary: amplitude[i] *= e^{i*phase(i)}.
    // This is how phase-oracles (e.g. Grover's "mark the solution") are
    // simulated directly — see apply_permutation()'s rationale above.
    void apply_diagonal_phase(const std::function<double(std::uint64_t)>& phase_fn);

    // Returns P(qubit == 1).
    double probability_one(int qubit) const;

    // <Z> expectation value for `qubit`, in [-1, 1] (= 1 - 2*P(qubit==1)).
    double expectation_z(int qubit) const;

    // Collapses `qubit` to a measured value (0 or 1) sampled from its
    // marginal distribution, renormalizing the remaining amplitudes.
    int measure_qubit(int qubit, std::mt19937_64& rng);

    // Measures every qubit at once (collapses to a single computational
    // basis state) and returns the resulting bitstring (qubit 0 = LSB).
    std::uint64_t measure_all(std::mt19937_64& rng);

    // Non-destructive probability of each computational basis state
    // (dimension() entries, index = bitstring value). Useful for tests and
    // for visualizing a state without consuming it via measurement.
    std::vector<double> outcome_probabilities() const;

    // Sum of |amplitude|^2 over the whole state; should stay ~1.0 after any
    // sequence of unitary operations (useful as a correctness sanity check).
    double total_probability() const;

private:
    int num_qubits_;
    std::vector<Complex> amplitudes_;

    void require_qubit(int qubit) const;
};

// A reusable, named sequence of gate operations that can be built once and
// applied to any QuantumState with a matching qubit count.
class QuantumCircuit {
public:
    explicit QuantumCircuit(int num_qubits);

    int num_qubits() const { return num_qubits_; }
    std::size_t gate_count() const { return ops_.size(); }

    QuantumCircuit& h(int qubit);
    QuantumCircuit& x(int qubit);
    QuantumCircuit& y(int qubit);
    QuantumCircuit& z(int qubit);
    QuantumCircuit& s(int qubit);
    QuantumCircuit& t(int qubit);
    QuantumCircuit& rx(int qubit, double theta);
    QuantumCircuit& ry(int qubit, double theta);
    QuantumCircuit& rz(int qubit, double theta);
    QuantumCircuit& phase(int qubit, double theta);
    QuantumCircuit& cnot(int control, int target);
    QuantumCircuit& cz(int control, int target);
    QuantumCircuit& controlled(const std::vector<int>& controls, int target, const Matrix2& gate, const std::string& label = "C-U");
    QuantumCircuit& toffoli(int control_a, int control_b, int target);
    QuantumCircuit& swap(int qubit_a, int qubit_b);

    // Runs every recorded operation against `state`, in order.
    void apply(QuantumState& state) const;

    // One line per gate, e.g. "H q0", "CNOT q0 -> q1", "TOFFOLI q0,q1 -> q2".
    std::string describe() const;

private:
    struct Operation {
        std::string label;
        std::vector<int> controls;
        int target = 0;
        Matrix2 gate;
        bool is_swap = false;
        int swap_other = -1;
    };

    int num_qubits_;
    std::vector<Operation> ops_;

    void require_qubit(int qubit) const;
};

// Applies the Quantum Fourier Transform (or its inverse) to `qubits` (given
// least-significant-qubit-first) in `state`, using the standard
// H + controlled-phase decomposition followed by a qubit-order reversal.
// This is true gate-level simulation (not a shortcut) since the QFT's
// amplitude interference is the actual source of Shor's/phase-estimation
// speedup.
void apply_qft(QuantumState& state, const std::vector<int>& qubits, bool inverse = false);

} // namespace quantum

#endif // COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_SIMULATOR_H
