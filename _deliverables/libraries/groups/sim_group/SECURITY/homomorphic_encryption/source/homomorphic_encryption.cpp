#include "../headers/homomorphic_encryption.h"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace security {

namespace {

std::uint64_t gcd(std::uint64_t left, std::uint64_t right) {
    while (right != 0) {
        const std::uint64_t remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

std::uint64_t lcm(std::uint64_t left, std::uint64_t right) {
    return (left / gcd(left, right)) * right;
}

std::uint64_t checked_square(std::uint64_t value) {
    if (value != 0 && value > std::numeric_limits<std::uint64_t>::max() / value) {
        throw std::invalid_argument("homomorphic key modulus is too large for uint64 arithmetic");
    }
    return value * value;
}

std::uint64_t modular_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t modulus) {
#if defined(__SIZEOF_INT128__)
    return static_cast<std::uint64_t>((static_cast<unsigned __int128>(left) * right) % modulus);
#else
    std::uint64_t result = 0;
    left %= modulus;
    while (right > 0) {
        if ((right & 1u) != 0) {
            result = result >= modulus - left ? result - (modulus - left) : result + left;
        }
        left = left >= modulus - left ? left - (modulus - left) : left + left;
        right >>= 1u;
    }
    return result;
#endif
}

std::uint64_t modular_power(std::uint64_t base, std::uint64_t exponent, std::uint64_t modulus) {
    std::uint64_t result = 1;
    base %= modulus;
    while (exponent > 0) {
        if ((exponent & 1u) != 0) {
            result = modular_multiply(result, base, modulus);
        }
        base = modular_multiply(base, base, modulus);
        exponent >>= 1u;
    }
    return result;
}

std::uint64_t modular_inverse(std::uint64_t value, std::uint64_t modulus) {
    if (modulus == 0) {
        throw std::invalid_argument("modular inverse requires a non-zero modulus");
    }

    std::int64_t previous_t = 0;
    std::int64_t current_t = 1;
    std::int64_t previous_r = static_cast<std::int64_t>(modulus);
    std::int64_t current_r = static_cast<std::int64_t>(value % modulus);

    while (current_r != 0) {
        const std::int64_t quotient = previous_r / current_r;
        const std::int64_t next_t = previous_t - quotient * current_t;
        previous_t = current_t;
        current_t = next_t;

        const std::int64_t next_r = previous_r - quotient * current_r;
        previous_r = current_r;
        current_r = next_r;
    }

    if (previous_r != 1) {
        throw std::invalid_argument("value has no modular inverse");
    }
    if (previous_t < 0) {
        previous_t += static_cast<std::int64_t>(modulus);
    }
    return static_cast<std::uint64_t>(previous_t);
}

bool is_prime(std::uint64_t value) {
    if (value < 2) return false;
    if (value % 2 == 0) return value == 2;
    for (std::uint64_t divisor = 3; divisor <= value / divisor; divisor += 2) {
        if (value % divisor == 0) return false;
    }
    return true;
}

std::uint64_t generate_prime(std::uint8_t bits, std::mt19937_64& rng) {
    if (bits < 3 || bits > 16) {
        throw std::invalid_argument("prime_bits must be between 3 and 16 for the uint64 Paillier simulator");
    }

    const std::uint64_t lower = 1ull << (bits - 1u);
    const std::uint64_t upper = (1ull << bits) - 1u;
    std::uniform_int_distribution<std::uint64_t> distribution(lower, upper);

    for (int attempt = 0; attempt < 10000; ++attempt) {
        std::uint64_t candidate = distribution(rng) | 1u;
        if (is_prime(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("failed to generate a prime for homomorphic key pair");
}

std::uint64_t paillier_l(std::uint64_t value, std::uint64_t n) {
    if ((value - 1u) % n != 0) {
        throw std::runtime_error("invalid Paillier L function input");
    }
    return (value - 1u) / n;
}

std::uint64_t seed_randomness() {
    std::random_device device;
    return (static_cast<std::uint64_t>(device()) << 32u) ^ static_cast<std::uint64_t>(device());
}

} // namespace

AdditiveHomomorphicEncryption::AdditiveHomomorphicEncryption()
    : AdditiveHomomorphicEncryption(generate_key_pair()) {}

AdditiveHomomorphicEncryption::AdditiveHomomorphicEncryption(const HomomorphicKeyPair& key_pair)
    : key_pair_(key_pair)
    , rng_(seed_randomness()) {
    if (key_pair_.public_key.n < 2 || key_pair_.public_key.n_squared != checked_square(key_pair_.public_key.n)) {
        throw std::invalid_argument("invalid homomorphic public key");
    }
    if (key_pair_.public_key.generator == 0 || key_pair_.private_key.lambda == 0 || key_pair_.private_key.mu == 0) {
        throw std::invalid_argument("invalid homomorphic key pair");
    }
}

HomomorphicKeyPair AdditiveHomomorphicEncryption::generate_key_pair(std::uint8_t prime_bits) {
    std::mt19937_64 rng(seed_randomness());
    std::uint64_t p = generate_prime(prime_bits, rng);
    std::uint64_t q = generate_prime(prime_bits, rng);
    while (q == p) {
        q = generate_prime(prime_bits, rng);
    }
    return key_pair_from_primes(p, q);
}

HomomorphicKeyPair AdditiveHomomorphicEncryption::key_pair_from_primes(std::uint64_t p, std::uint64_t q) {
    if (!is_prime(p) || !is_prime(q) || p == q) {
        throw std::invalid_argument("key_pair_from_primes requires two distinct primes");
    }
    if (p > std::numeric_limits<std::uint64_t>::max() / q) {
        throw std::invalid_argument("homomorphic key primes produce an overflowing modulus");
    }

    HomomorphicKeyPair key_pair;
    key_pair.public_key.n = p * q;
    key_pair.public_key.n_squared = checked_square(key_pair.public_key.n);
    key_pair.public_key.generator = key_pair.public_key.n + 1u;
    key_pair.private_key.lambda = lcm(p - 1u, q - 1u);

    const std::uint64_t powered = modular_power(key_pair.public_key.generator,
                                                key_pair.private_key.lambda,
                                                key_pair.public_key.n_squared);
    key_pair.private_key.mu = modular_inverse(paillier_l(powered, key_pair.public_key.n),
                                              key_pair.public_key.n);
    return key_pair;
}

HomomorphicCiphertext AdditiveHomomorphicEncryption::encrypt(std::uint64_t plaintext) {
    return encrypt_with_randomness(plaintext, random_coprime_to_modulus());
}

HomomorphicCiphertext AdditiveHomomorphicEncryption::encrypt_with_randomness(std::uint64_t plaintext,
                                                                              std::uint64_t randomness) const {
    validate_plaintext(plaintext);
    if (randomness == 0 || randomness >= key_pair_.public_key.n || gcd(randomness, key_pair_.public_key.n) != 1) {
        throw std::invalid_argument("encryption randomness must be coprime to the public modulus");
    }

    const auto& key = key_pair_.public_key;
    const std::uint64_t encoded_message = modular_power(key.generator, plaintext, key.n_squared);
    const std::uint64_t encoded_randomness = modular_power(randomness, key.n, key.n_squared);
    return {modular_multiply(encoded_message, encoded_randomness, key.n_squared)};
}

std::uint64_t AdditiveHomomorphicEncryption::decrypt(const HomomorphicCiphertext& ciphertext) const {
    validate_ciphertext(ciphertext);
    const auto& public_key_ref = key_pair_.public_key;
    const auto& private_key_ref = key_pair_.private_key;
    const std::uint64_t powered = modular_power(ciphertext.value,
                                                private_key_ref.lambda,
                                                public_key_ref.n_squared);
    const std::uint64_t message = modular_multiply(paillier_l(powered, public_key_ref.n),
                                                   private_key_ref.mu,
                                                   public_key_ref.n);
    return message;
}

HomomorphicCiphertext AdditiveHomomorphicEncryption::add(const HomomorphicCiphertext& left,
                                                          const HomomorphicCiphertext& right) const {
    validate_ciphertext(left);
    validate_ciphertext(right);
    return {modular_multiply(left.value, right.value, key_pair_.public_key.n_squared)};
}

HomomorphicCiphertext AdditiveHomomorphicEncryption::add_plaintext(const HomomorphicCiphertext& ciphertext,
                                                                    std::uint64_t plaintext) const {
    validate_ciphertext(ciphertext);
    validate_plaintext(plaintext);
    const std::uint64_t encoded_plaintext = modular_power(key_pair_.public_key.generator,
                                                          plaintext,
                                                          key_pair_.public_key.n_squared);
    return {modular_multiply(ciphertext.value, encoded_plaintext, key_pair_.public_key.n_squared)};
}

HomomorphicCiphertext AdditiveHomomorphicEncryption::multiply_plaintext(const HomomorphicCiphertext& ciphertext,
                                                                        std::uint64_t scalar) const {
    validate_ciphertext(ciphertext);
    return {modular_power(ciphertext.value, scalar % key_pair_.public_key.n, key_pair_.public_key.n_squared)};
}

std::string AdditiveHomomorphicEncryption::describe() const {
    std::ostringstream output;
    output << "Paillier additive homomorphic encryption simulator (modulus n="
           << key_pair_.public_key.n << ")";
    return output.str();
}

std::uint64_t AdditiveHomomorphicEncryption::random_coprime_to_modulus() {
    std::uniform_int_distribution<std::uint64_t> distribution(1u, key_pair_.public_key.n - 1u);
    for (int attempt = 0; attempt < 10000; ++attempt) {
        const std::uint64_t candidate = distribution(rng_);
        if (gcd(candidate, key_pair_.public_key.n) == 1) {
            return candidate;
        }
    }
    throw std::runtime_error("failed to generate encryption randomness");
}

void AdditiveHomomorphicEncryption::validate_plaintext(std::uint64_t plaintext) const {
    if (plaintext >= key_pair_.public_key.n) {
        throw std::out_of_range("plaintext must be smaller than the public modulus");
    }
}

void AdditiveHomomorphicEncryption::validate_ciphertext(const HomomorphicCiphertext& ciphertext) const {
    if (ciphertext.value == 0 || ciphertext.value >= key_pair_.public_key.n_squared) {
        throw std::out_of_range("ciphertext is outside the public modulus squared range");
    }
}

} // namespace security