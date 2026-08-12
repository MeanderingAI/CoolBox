#include "homomorphic_encryption.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

int main() {
    using security::AdditiveHomomorphicEncryption;

    const auto key_pair = AdditiveHomomorphicEncryption::key_pair_from_primes(53, 59);
    AdditiveHomomorphicEncryption encryption(key_pair);

    const auto encrypted_7 = encryption.encrypt_with_randomness(7, 3);
    const auto encrypted_11 = encryption.encrypt_with_randomness(11, 5);

    assert(encryption.decrypt(encrypted_7) == 7);
    assert(encryption.decrypt(encrypted_11) == 11);

    const auto encrypted_sum = encryption.add(encrypted_7, encrypted_11);
    assert(encryption.decrypt(encrypted_sum) == 18);

    const auto encrypted_plus_plaintext = encryption.add_plaintext(encrypted_7, 15);
    assert(encryption.decrypt(encrypted_plus_plaintext) == 22);

    const auto encrypted_scaled = encryption.multiply_plaintext(encrypted_11, 4);
    assert(encryption.decrypt(encrypted_scaled) == 44);

    const auto wrap_left = encryption.encrypt_with_randomness(key_pair.public_key.n - 2, 7);
    const auto wrap_right = encryption.encrypt_with_randomness(5, 11);
    assert(encryption.decrypt(encryption.add(wrap_left, wrap_right)) == 3);

    bool rejected_invalid_plaintext = false;
    try {
        encryption.encrypt(key_pair.public_key.n);
    } catch (const std::out_of_range&) {
        rejected_invalid_plaintext = true;
    }
    assert(rejected_invalid_plaintext);

    std::cout << "homomorphic_encryption tests passed\n";
    return 0;
}