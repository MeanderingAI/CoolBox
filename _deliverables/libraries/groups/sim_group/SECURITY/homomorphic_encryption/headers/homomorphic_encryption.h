#pragma once

#include <cstdint>
#include <random>
#include <string>

namespace security {

struct HomomorphicPublicKey {
    std::uint64_t n = 0;
    std::uint64_t n_squared = 0;
    std::uint64_t generator = 0;
};

struct HomomorphicPrivateKey {
    std::uint64_t lambda = 0;
    std::uint64_t mu = 0;
};

struct HomomorphicKeyPair {
    HomomorphicPublicKey public_key;
    HomomorphicPrivateKey private_key;
};

struct HomomorphicCiphertext {
    std::uint64_t value = 0;
};

class AdditiveHomomorphicEncryption {
public:
    AdditiveHomomorphicEncryption();
    explicit AdditiveHomomorphicEncryption(const HomomorphicKeyPair& key_pair);

    static HomomorphicKeyPair generate_key_pair(std::uint8_t prime_bits = 16);
    static HomomorphicKeyPair key_pair_from_primes(std::uint64_t p, std::uint64_t q);

    const HomomorphicPublicKey& public_key() const { return key_pair_.public_key; }

    HomomorphicCiphertext encrypt(std::uint64_t plaintext);
    HomomorphicCiphertext encrypt_with_randomness(std::uint64_t plaintext, std::uint64_t randomness) const;
    std::uint64_t decrypt(const HomomorphicCiphertext& ciphertext) const;

    HomomorphicCiphertext add(const HomomorphicCiphertext& left,
                              const HomomorphicCiphertext& right) const;
    HomomorphicCiphertext add_plaintext(const HomomorphicCiphertext& ciphertext,
                                        std::uint64_t plaintext) const;
    HomomorphicCiphertext multiply_plaintext(const HomomorphicCiphertext& ciphertext,
                                             std::uint64_t scalar) const;

    std::string describe() const;

private:
    HomomorphicKeyPair key_pair_;
    std::mt19937_64 rng_;

    std::uint64_t random_coprime_to_modulus();
    void validate_plaintext(std::uint64_t plaintext) const;
    void validate_ciphertext(const HomomorphicCiphertext& ciphertext) const;
};

} // namespace security