#include "../headers/quantum_simulator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace quantum {

QuantumState::QuantumState(int num_qubits) : num_qubits_(num_qubits) {
    if (num_qubits <= 0) throw std::invalid_argument("QuantumState: num_qubits must be positive");
    if (num_qubits > kMaxQubits) {
        throw std::invalid_argument("QuantumState: num_qubits exceeds kMaxQubits (" +
                                    std::to_string(kMaxQubits) + ") -- the dense state vector "
                                    "would require too much memory");
    }
    amplitudes_.assign(static_cast<std::size_t>(1) << num_qubits, Complex(0.0, 0.0));
    amplitudes_[0] = Complex(1.0, 0.0); // |00...0>
}

void QuantumState::require_qubit(int qubit) const {
    if (qubit < 0 || qubit >= num_qubits_) {
        throw std::out_of_range("QuantumState: qubit index " + std::to_string(qubit) +
                                " out of range for a " + std::to_string(num_qubits_) + "-qubit state");
    }
}

void QuantumState::apply_single_qubit_gate(int qubit, const Matrix2& gate) {
    apply_multi_controlled_gate({}, qubit, gate);
}

void QuantumState::apply_controlled_gate(int control, int target, const Matrix2& gate) {
    apply_multi_controlled_gate({control}, target, gate);
}

void QuantumState::apply_multi_controlled_gate(const std::vector<int>& controls, int target, const Matrix2& gate) {
    require_qubit(target);
    for (int c : controls) {
        require_qubit(c);
        if (c == target) throw std::invalid_argument("QuantumState: control qubit cannot equal target qubit");
    }

    std::uint64_t control_mask = 0;
    for (int c : controls) control_mask |= (std::uint64_t{1} << c);
    const std::uint64_t target_mask = std::uint64_t{1} << target;
    const std::uint64_t dim = amplitudes_.size();

    for (std::uint64_t i = 0; i < dim; ++i) {
        if (i & target_mask) continue;                       // only visit the target=0 half of each pair
        if ((i & control_mask) != control_mask) continue;     // all controls must be |1>
        const std::uint64_t i1 = i | target_mask;
        const Complex a0 = amplitudes_[i];
        const Complex a1 = amplitudes_[i1];
        amplitudes_[i]  = gate.m00 * a0 + gate.m01 * a1;
        amplitudes_[i1] = gate.m10 * a0 + gate.m11 * a1;
    }
}

void QuantumState::apply_swap(int qubit_a, int qubit_b) {
    require_qubit(qubit_a);
    require_qubit(qubit_b);
    if (qubit_a == qubit_b) return;

    const std::uint64_t mask_a = std::uint64_t{1} << qubit_a;
    const std::uint64_t mask_b = std::uint64_t{1} << qubit_b;
    const std::uint64_t dim = amplitudes_.size();

    for (std::uint64_t i = 0; i < dim; ++i) {
        const bool bit_a = (i & mask_a) != 0;
        const bool bit_b = (i & mask_b) != 0;
        if (bit_a && !bit_b) { // canonical direction: visit each differing pair exactly once
            const std::uint64_t j = (i & ~mask_a) | mask_b;
            std::swap(amplitudes_[i], amplitudes_[j]);
        }
    }
}

void QuantumState::apply_permutation(const std::function<std::uint64_t(std::uint64_t)>& permute) {
    const std::uint64_t dim = amplitudes_.size();
    std::vector<Complex> new_amplitudes(dim, Complex(0.0, 0.0));
    for (std::uint64_t i = 0; i < dim; ++i) {
        const std::uint64_t j = permute(i);
        if (j >= dim) throw std::out_of_range("QuantumState::apply_permutation: permute() returned an out-of-range index");
        new_amplitudes[j] = amplitudes_[i];
    }
    amplitudes_ = std::move(new_amplitudes);
}

void QuantumState::apply_diagonal_phase(const std::function<double(std::uint64_t)>& phase_fn) {
    const std::uint64_t dim = amplitudes_.size();
    for (std::uint64_t i = 0; i < dim; ++i) {
        const double theta = phase_fn(i);
        if (theta != 0.0) {
            amplitudes_[i] *= Complex(std::cos(theta), std::sin(theta));
        }
    }
}

double QuantumState::probability_one(int qubit) const {
    require_qubit(qubit);
    const std::uint64_t mask = std::uint64_t{1} << qubit;
    double total = 0.0;
    for (std::uint64_t i = 0; i < amplitudes_.size(); ++i) {
        if (i & mask) total += std::norm(amplitudes_[i]);
    }
    return total;
}

double QuantumState::expectation_z(int qubit) const {
    return 1.0 - 2.0 * probability_one(qubit);
}

int QuantumState::measure_qubit(int qubit, std::mt19937_64& rng) {
    require_qubit(qubit);
    const double p1 = probability_one(qubit);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    const int outcome = (dist(rng) < p1) ? 1 : 0;

    const std::uint64_t mask = std::uint64_t{1} << qubit;
    double norm_sq = 0.0;
    for (std::uint64_t i = 0; i < amplitudes_.size(); ++i) {
        const bool bit = (i & mask) != 0;
        if (static_cast<int>(bit) == outcome) {
            norm_sq += std::norm(amplitudes_[i]);
        } else {
            amplitudes_[i] = Complex(0.0, 0.0);
        }
    }
    const double norm = std::sqrt(norm_sq);
    if (norm > 0.0) {
        for (auto& amp : amplitudes_) amp /= norm;
    }
    return outcome;
}

std::uint64_t QuantumState::measure_all(std::mt19937_64& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    const double r = dist(rng);
    double cumulative = 0.0;
    std::uint64_t chosen = amplitudes_.size() - 1; // fallback for floating-point edge case
    for (std::uint64_t i = 0; i < amplitudes_.size(); ++i) {
        cumulative += std::norm(amplitudes_[i]);
        if (r < cumulative) { chosen = i; break; }
    }
    std::fill(amplitudes_.begin(), amplitudes_.end(), Complex(0.0, 0.0));
    amplitudes_[chosen] = Complex(1.0, 0.0);
    return chosen;
}

std::vector<double> QuantumState::outcome_probabilities() const {
    std::vector<double> probs(amplitudes_.size());
    for (std::size_t i = 0; i < amplitudes_.size(); ++i) probs[i] = std::norm(amplitudes_[i]);
    return probs;
}

double QuantumState::total_probability() const {
    double total = 0.0;
    for (const auto& amp : amplitudes_) total += std::norm(amp);
    return total;
}

} // namespace quantum
