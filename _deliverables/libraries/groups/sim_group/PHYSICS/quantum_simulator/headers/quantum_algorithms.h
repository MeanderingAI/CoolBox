#ifndef COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_ALGORITHMS_H
#define COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_ALGORITHMS_H

// quantum_algorithms — classic textbook algorithms built on top of
// quantum_simulator.h: entangling demo states (Bell, GHZ), Deutsch-Jozsa,
// Grover's search, and Shor's factoring algorithm.
//
// A note on "oracles": Deutsch-Jozsa, Grover's, and Shor's all involve a
// black-box classical function wrapped in a unitary ("the oracle"). Rather
// than hand-compiling each oracle into elementary reversible gates (adders,
// carry chains, ...) — which is a circuit-synthesis problem orthogonal to
// simulating quantum mechanics — this library applies the oracle's *exact*
// unitary action directly to the state vector via
// QuantumState::apply_permutation()/apply_diagonal_phase(). This is
// mathematically identical to what a gate-level circuit would compute; the
// simulated quantum behaviour (superposition, entanglement, interference)
// is fully real and not shortcut. This is also the standard approach used
// by e.g. Qiskit's own textbook Shor's implementation.

#include "quantum_simulator.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <utility>

namespace quantum {
namespace algorithms {

// Builds the 2-qubit Bell state |Phi+> = (|00> + |11>) / sqrt(2).
QuantumCircuit bell_state();

// Builds an n-qubit GHZ state (|00...0> + |11...1>) / sqrt(2).
QuantumCircuit ghz_state(int num_qubits);

// A classical Boolean function f: {0,1}^n -> {0,1}, represented as a
// function from an n-bit input (packed into a std::uint64_t) to 0 or 1.
using BooleanOracle = std::function<int(std::uint64_t)>;

// Runs the Deutsch-Jozsa state preparation (assumes `oracle` is either
// constant or balanced) and returns the final (num_input_qubits + 1)-qubit
// state, just before the input register would be measured. The ancilla is
// the highest-indexed qubit (index num_input_qubits). Exposed separately
// from deutsch_jozsa() so tests/visualizations can inspect
// outcome_probabilities() deterministically instead of relying on a single
// random sample.
QuantumState deutsch_jozsa_state(int num_input_qubits, const BooleanOracle& oracle);

// Convenience wrapper: prepares the state, measures the input register, and
// returns true if `oracle` was constant, false if it was balanced.
bool deutsch_jozsa(int num_input_qubits, const BooleanOracle& oracle, std::mt19937_64& rng);

// Returns the standard optimal number of Grover iterations for an N = 2^n
// element search space with exactly one marked item: floor(pi/4 * sqrt(N)),
// clamped to at least 1.
int optimal_grover_iterations(int num_qubits);

// Runs Grover's algorithm for `num_qubits` qubits with the single marked
// basis state `marked_index`, and returns the state right before the final
// measurement (after the optimal number of Grover iterations).
QuantumState grover_state(int num_qubits, std::uint64_t marked_index);

// Convenience wrapper: prepares the state and samples a single measurement
// (ideally == marked_index with high probability).
std::uint64_t grover_search(int num_qubits, std::uint64_t marked_index, std::mt19937_64& rng);

// ── Shor's algorithm ──────────────────────────────────────────────────────

// One attempt's worth of diagnostic detail, useful for a UI/CLI to narrate
// what the algorithm is doing step by step.
struct ShorAttempt {
    std::uint64_t a = 0;             // randomly chosen base
    std::uint64_t gcd_a_n = 0;       // gcd(a, N); >1 means a lucky classical shortcut
    int n_count_qubits = 0;          // counting-register width used
    int n_work_qubits = 0;           // work-register width used
    std::uint64_t measured_value = 0; // raw counting-register measurement
    std::uint64_t period = 0;        // period r extracted via continued fractions (0 = failed)
    bool succeeded = false;          // true if this attempt yielded a nontrivial factor
    std::uint64_t factor = 0;        // a nontrivial factor of N, if succeeded
};

struct ShorResult {
    std::uint64_t n = 0;
    bool succeeded = false;
    std::uint64_t factor_a = 0;
    std::uint64_t factor_b = 0;
    std::vector<ShorAttempt> attempts; // one entry per attempt, in order
};

// Factors `n` using Shor's algorithm (quantum order-finding via simulated
// phase estimation + QFT, with classical pre/post-processing). `n` must be
// an odd composite >= 15 that is not a prime power; even numbers and prime
// powers are rejected with std::invalid_argument since Shor's quantum
// subroutine isn't the right tool for them (classical checks handle those
// trivially in a real implementation, but this simulator focuses on the
// genuinely quantum case). Keep `n` small (<= ~63) for interactive runtimes
// — qubit count grows as ~3*log2(n), so the simulated state vector grows
// exponentially with n's bit-width.
ShorResult shor_factor(std::uint64_t n, std::mt19937_64& rng, int max_attempts = 10);

// Modular exponentiation (base^exp mod modulus) via fast exponentiation by
// squaring. Exposed because it's independently useful (and independently
// testable) classical plumbing that shor_factor() relies on.
std::uint64_t mod_pow(std::uint64_t base, std::uint64_t exp, std::uint64_t modulus);

} // namespace algorithms
} // namespace quantum

#endif // COOLBOX_SIM_GROUP_PHYSICS_QUANTUM_ALGORITHMS_H
