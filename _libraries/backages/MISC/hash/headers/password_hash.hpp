#ifndef COOLBOX__LIBRARIES_BACKAGES_MISC_HASH_HEADERS_PASSWORD_HASH_HPP
#define COOLBOX__LIBRARIES_BACKAGES_MISC_HASH_HEADERS_PASSWORD_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace utils {
namespace hash {

using ByteVector = std::vector<std::uint8_t>;

struct PBKDF2Params {
    std::uint32_t iterations = 600000;
    std::size_t output_length = 32;
    std::size_t salt_length = 16;
};

struct ScryptParams {
    std::uint64_t cost = 1u << 15;
    std::uint32_t block_size = 8;
    std::uint32_t parallelization = 1;
    std::size_t output_length = 32;
    std::size_t salt_length = 16;
};

struct BCryptParams {
    std::uint32_t cost = 12;
};

struct Argon2idParams {
    std::uint32_t iterations = 3;
    std::uint32_t memory_kib = 65536;
    std::uint32_t parallelism = 1;
    std::size_t output_length = 32;
    std::size_t salt_length = 16;
};

struct PasswordHashComparison {
    std::string name;
    std::string best_for;
    std::string tradeoffs;
};

std::vector<PasswordHashComparison> compare_password_hashers();

bool supports_pbkdf2_sha256();
bool supports_scrypt();
bool supports_bcrypt();
bool supports_argon2id();

ByteVector random_salt(std::size_t length = 16);
std::string hex_encode(const ByteVector& bytes);

ByteVector pbkdf2_sha256_derive(const std::string& password,
                                const ByteVector& salt,
                                std::uint32_t iterations,
                                std::size_t output_length);

std::string pbkdf2_sha256_hash(const std::string& password,
                               const PBKDF2Params& params = {});

bool verify_pbkdf2_sha256(const std::string& password,
                          const std::string& encoded_hash);

ByteVector scrypt_derive(const std::string& password,
                         const ByteVector& salt,
                         std::uint64_t cost,
                         std::uint32_t block_size,
                         std::uint32_t parallelization,
                         std::size_t output_length);

std::string scrypt_hash(const std::string& password,
                        const ScryptParams& params = {});

bool verify_scrypt(const std::string& password,
                   const std::string& encoded_hash);

std::string bcrypt_hash(const std::string& password,
                        const BCryptParams& params = {});

bool verify_bcrypt(const std::string& password,
                   const std::string& encoded_hash);

std::string argon2id_hash(const std::string& password,
                          const Argon2idParams& params = {});

bool verify_argon2id(const std::string& password,
                     const std::string& encoded_hash);

} // namespace hash
} // namespace utils
#endif  // COOLBOX__LIBRARIES_BACKAGES_MISC_HASH_HEADERS_PASSWORD_HASH_HPP
