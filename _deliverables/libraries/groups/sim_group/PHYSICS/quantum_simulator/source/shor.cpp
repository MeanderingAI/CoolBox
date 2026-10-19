#include "../headers/quantum_algorithms.h"

#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>

namespace quantum {
namespace algorithms {

std::uint64_t mod_pow(std::uint64_t base, std::uint64_t exp, std::uint64_t modulus) {
    if (modulus == 1) return 0;
    std::uint64_t result = 1;
    base %= modulus;
    while (exp > 0) {
        if (exp & 1ULL) result = (result * base) % modulus;
        base = (base * base) % modulus;
        exp >>= 1;
    }
    return result;
}

namespace {

bool is_prime(std::uint64_t n) {
    if (n < 2) return false;
    for (std::uint64_t d = 2; d * d <= n; ++d) {
        if (n % d == 0) return false;
    }
    return true;
}

// Returns a nontrivial base p such that p^k == n for some k >= 2, or 0 if n
// is not a perfect power. Shor's quantum subroutine can't reliably factor
// prime powers (every nontrivial order of every base ends up giving a
// trivial gcd), so this classical pre-check short-circuits that case.
std::uint64_t perfect_power_base(std::uint64_t n) {
    for (std::uint64_t k = 2; (std::uint64_t{1} << k) <= n; ++k) {
        const double approx_root = std::pow(static_cast<double>(n), 1.0 / static_cast<double>(k));
        for (std::int64_t delta = -1; delta <= 1; ++delta) {
            const std::int64_t candidate_signed = static_cast<std::int64_t>(std::llround(approx_root)) + delta;
            if (candidate_signed < 2) continue;
            const std::uint64_t candidate = static_cast<std::uint64_t>(candidate_signed);
            std::uint64_t power = 1;
            bool overflowed = false;
            for (std::uint64_t i = 0; i < k; ++i) {
                if (power > n / candidate + 1) { overflowed = true; break; } // cheap overflow guard
                power *= candidate;
            }
            if (!overflowed && power == n) return candidate;
        }
    }
    return 0;
}

// Applies the "multiply the work register by `multiplier` mod `modulus`"
// unitary, controlled on `control_qubit`. See quantum_simulator.h's
// apply_permutation() doc comment for why a direct permutation is a
// legitimate (and exact) way to simulate this, instead of a hand-compiled
// reversible arithmetic circuit. This permutation is only valid when
// gcd(multiplier, modulus) == 1 (guaranteed by Shor's algorithm's own
// classical precondition that gcd(a, N) == 1 before the quantum subroutine
// ever runs, since powers of a unit are units).
void apply_controlled_modular_multiply(QuantumState& state, int control_qubit,
                                       const std::vector<int>& work_qubits,
                                       std::uint64_t multiplier, std::uint64_t modulus) {
    const std::uint64_t control_mask = std::uint64_t{1} << control_qubit;
    std::uint64_t work_mask = 0;
    for (int q : work_qubits) work_mask |= (std::uint64_t{1} << q);

    state.apply_permutation([&](std::uint64_t i) -> std::uint64_t {
        if ((i & control_mask) == 0) return i; // control = 0: identity

        std::uint64_t y = 0;
        for (std::size_t b = 0; b < work_qubits.size(); ++b) {
            if (i & (std::uint64_t{1} << work_qubits[b])) y |= (std::uint64_t{1} << b);
        }
        const std::uint64_t new_y = (y < modulus) ? (y * multiplier) % modulus : y;

        std::uint64_t result = i & ~work_mask;
        for (std::size_t b = 0; b < work_qubits.size(); ++b) {
            if (new_y & (std::uint64_t{1} << b)) result |= (std::uint64_t{1} << work_qubits[b]);
        }
        return result;
    });
}

struct OrderFindingOutcome {
    std::uint64_t measured = 0;
    int n_count = 0;
    int n_work = 0;
};

OrderFindingOutcome find_order_once(std::uint64_t a, std::uint64_t n, std::mt19937_64& rng) {
    int n_work = 1;
    while ((std::uint64_t{1} << n_work) < n) ++n_work;
    int n_count = 2 * n_work;
    while (n_count + n_work > QuantumState::kMaxQubits && n_count > 1) --n_count;
    if (n_count + n_work > QuantumState::kMaxQubits) {
        throw std::runtime_error("shor_factor: n is too large for this simulator's qubit budget");
    }

    const int total_qubits = n_count + n_work;
    QuantumState state(total_qubits);

    std::vector<int> work_qubits(static_cast<std::size_t>(n_work));
    for (int i = 0; i < n_work; ++i) work_qubits[static_cast<std::size_t>(i)] = n_count + i;

    // Work register starts at |1> (the multiplicative identity).
    state.apply_single_qubit_gate(work_qubits[0], gates::pauli_x());

    // Uniform superposition over the counting register.
    for (int q = 0; q < n_count; ++q) state.apply_single_qubit_gate(q, gates::hadamard());

    // Controlled modular exponentiation: counting qubit j controls a
    // multiply-by-(a^(2^j) mod n) on the work register.
    for (int j = 0; j < n_count; ++j) {
        const std::uint64_t power = mod_pow(a, std::uint64_t{1} << j, n);
        apply_controlled_modular_multiply(state, j, work_qubits, power, n);
    }

    std::vector<int> counting_qubits(static_cast<std::size_t>(n_count));
    for (int i = 0; i < n_count; ++i) counting_qubits[static_cast<std::size_t>(i)] = i;
    apply_qft(state, counting_qubits, /*inverse=*/true);

    std::uint64_t measured = 0;
    for (int q = 0; q < n_count; ++q) {
        const int bit = state.measure_qubit(q, rng);
        measured |= (static_cast<std::uint64_t>(bit) << q);
    }
    return {measured, n_count, n_work};
}

// Continued-fraction expansion of measured/2^n_count, checking each
// convergent denominator directly against a^q == 1 (mod n) rather than
// trusting the first convergent blindly -- the standard, robust way to
// post-process a noisy/imperfect phase-estimation measurement in Shor's
// algorithm.
std::uint64_t extract_period(std::uint64_t measured, std::uint64_t q_max, std::uint64_t a, std::uint64_t n) {
    if (measured == 0) return 0;

    std::uint64_t num = measured, den = q_max;
    std::uint64_t h_prev2 = 0, h_prev1 = 1;
    std::uint64_t k_prev2 = 1, k_prev1 = 0;

    while (den != 0) {
        const std::uint64_t term = num / den;
        const std::uint64_t h_cur = term * h_prev1 + h_prev2;
        const std::uint64_t k_cur = term * k_prev1 + k_prev2;
        if (k_cur >= n) break;
        if (k_cur > 0 && mod_pow(a, k_cur, n) == 1) return k_cur;

        const std::uint64_t rem = num % den;
        num = den;
        den = rem;
        h_prev2 = h_prev1; h_prev1 = h_cur;
        k_prev2 = k_prev1; k_prev1 = k_cur;
    }
    return 0;
}

} // namespace

ShorResult shor_factor(std::uint64_t n, std::mt19937_64& rng, int max_attempts) {
    if (n < 15) throw std::invalid_argument("shor_factor: n must be >= 15");
    if (n % 2 == 0) throw std::invalid_argument("shor_factor: n must be odd (2 is a trivial factor otherwise)");
    if (is_prime(n)) throw std::invalid_argument("shor_factor: n must be composite");

    ShorResult result;
    result.n = n;

    if (const std::uint64_t base = perfect_power_base(n); base != 0) {
        ShorAttempt rec;
        rec.succeeded = true;
        rec.factor = base;
        result.attempts.push_back(rec);
        result.succeeded = true;
        result.factor_a = base;
        result.factor_b = n / base;
        return result;
    }

    std::uniform_int_distribution<std::uint64_t> dist(2, n - 2);
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        ShorAttempt rec;
        const std::uint64_t a = dist(rng);
        rec.a = a;
        const std::uint64_t g = std::gcd(a, n);
        rec.gcd_a_n = g;

        if (g > 1) {
            rec.succeeded = true;
            rec.factor = g;
            result.attempts.push_back(rec);
            result.succeeded = true;
            result.factor_a = g;
            result.factor_b = n / g;
            return result;
        }

        const OrderFindingOutcome outcome = find_order_once(a, n, rng);
        rec.n_count_qubits = outcome.n_count;
        rec.n_work_qubits = outcome.n_work;
        rec.measured_value = outcome.measured;

        const std::uint64_t q_max = std::uint64_t{1} << outcome.n_count;
        const std::uint64_t r = extract_period(outcome.measured, q_max, a, n);
        rec.period = r;

        if (r == 0 || (r % 2 != 0)) {
            result.attempts.push_back(rec);
            continue;
        }
        const std::uint64_t half = mod_pow(a, r / 2, n);
        if (half == n - 1) { // a^(r/2) == -1 (mod n): trivial, must retry
            result.attempts.push_back(rec);
            continue;
        }

        const std::uint64_t f1 = std::gcd(half - 1, n);
        const std::uint64_t f2 = std::gcd(half + 1, n);
        if (f1 > 1 && f1 < n) {
            rec.succeeded = true;
            rec.factor = f1;
            result.attempts.push_back(rec);
            result.succeeded = true;
            result.factor_a = f1;
            result.factor_b = n / f1;
            return result;
        }
        if (f2 > 1 && f2 < n) {
            rec.succeeded = true;
            rec.factor = f2;
            result.attempts.push_back(rec);
            result.succeeded = true;
            result.factor_a = f2;
            result.factor_b = n / f2;
            return result;
        }
        result.attempts.push_back(rec);
    }

    result.succeeded = false;
    return result;
}

} // namespace algorithms
} // namespace quantum
