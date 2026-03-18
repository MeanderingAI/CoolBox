#include "password_hash.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>

#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#endif

#if defined(COOLBOX_HASH_USE_ARGON2_PROVIDER)
#include <argon2.h>
#endif

#if defined(COOLBOX_HASH_USE_CRYPT_PROVIDER)
#if __has_include(<crypt.h>)
#include <crypt.h>
#endif
#include <unistd.h>
#endif

namespace utils {
namespace hash {
namespace {

constexpr char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char kBcryptBase64Alphabet[] =
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

[[nodiscard]] bool constant_time_equal(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }

    unsigned char diff = 0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        diff |= static_cast<unsigned char>(left[i] ^ right[i]);
    }
    return diff == 0;
}

[[nodiscard]] bool constant_time_equal(const ByteVector& left, const ByteVector& right) {
    if (left.size() != right.size()) {
        return false;
    }

    std::uint8_t diff = 0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        diff |= static_cast<std::uint8_t>(left[i] ^ right[i]);
    }
    return diff == 0;
}

[[nodiscard]] std::string base64_encode(const ByteVector& input) {
    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    std::uint32_t accumulator = 0;
    int bit_count = 0;
    for (std::uint8_t byte : input) {
        accumulator = (accumulator << 8u) | byte;
        bit_count += 8;
        while (bit_count >= 6) {
            bit_count -= 6;
            encoded.push_back(kBase64Alphabet[(accumulator >> bit_count) & 0x3fu]);
        }
    }

    if (bit_count > 0) {
        accumulator <<= static_cast<std::uint32_t>(6 - bit_count);
        encoded.push_back(kBase64Alphabet[accumulator & 0x3fu]);
    }

    while (encoded.size() % 4 != 0) {
        encoded.push_back('=');
    }

    return encoded;
}

[[nodiscard]] ByteVector base64_decode(std::string_view input) {
    std::array<int, 256> lookup{};
    lookup.fill(-1);
    for (int i = 0; i < 64; ++i) {
        lookup[static_cast<unsigned char>(kBase64Alphabet[i])] = i;
    }

    ByteVector output;
    std::uint32_t accumulator = 0;
    int bit_count = 0;
    for (char ch : input) {
        if (ch == '=') {
            break;
        }
        const int value = lookup[static_cast<unsigned char>(ch)];
        if (value < 0) {
            throw std::invalid_argument("Invalid base64 character encountered");
        }
        accumulator = (accumulator << 6u) | static_cast<std::uint32_t>(value);
        bit_count += 6;
        if (bit_count >= 8) {
            bit_count -= 8;
            output.push_back(static_cast<std::uint8_t>((accumulator >> bit_count) & 0xffu));
        }
    }

    return output;
}

[[nodiscard]] std::string bcrypt_base64_encode(const ByteVector& input) {
    std::string encoded;
    encoded.reserve(((input.size() * 8) + 5) / 6);

    std::uint32_t accumulator = 0;
    int bit_count = 0;
    for (std::uint8_t byte : input) {
        accumulator = (accumulator << 8u) | byte;
        bit_count += 8;
        while (bit_count >= 6) {
            bit_count -= 6;
            encoded.push_back(kBcryptBase64Alphabet[(accumulator >> bit_count) & 0x3fu]);
        }
    }

    if (bit_count > 0) {
        accumulator <<= static_cast<std::uint32_t>(6 - bit_count);
        encoded.push_back(kBcryptBase64Alphabet[accumulator & 0x3fu]);
    }

    return encoded;
}

[[nodiscard]] std::vector<std::string> split(std::string_view text, char delimiter) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.emplace_back(text.substr(start));
            break;
        }
        parts.emplace_back(text.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

[[nodiscard]] std::uint32_t parse_u32(const std::string& text, const char* field_name) {
    std::size_t consumed = 0;
    const unsigned long value = std::stoul(text, &consumed, 10);
    if (consumed != text.size() || value > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string("Invalid ") + field_name + " value");
    }
    return static_cast<std::uint32_t>(value);
}

[[nodiscard]] std::uint64_t parse_u64(const std::string& text, const char* field_name) {
    std::size_t consumed = 0;
    const unsigned long long value = std::stoull(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::invalid_argument(std::string("Invalid ") + field_name + " value");
    }
    return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::string to_pbkdf2_encoding(std::uint32_t iterations,
                                             const ByteVector& salt,
                                             const ByteVector& derived) {
    return "$coolbox-pbkdf2-sha256$" + std::to_string(iterations) + "$" +
           base64_encode(salt) + "$" + base64_encode(derived);
}

[[nodiscard]] std::string to_scrypt_encoding(std::uint64_t cost,
                                             std::uint32_t block_size,
                                             std::uint32_t parallelization,
                                             const ByteVector& salt,
                                             const ByteVector& derived) {
    return "$coolbox-scrypt$" + std::to_string(cost) + "$" + std::to_string(block_size) +
           "$" + std::to_string(parallelization) + "$" + base64_encode(salt) + "$" +
           base64_encode(derived);
}

[[nodiscard]] std::string crypt_string(const std::string& password, const std::string& salt_or_hash) {
#if defined(COOLBOX_HASH_USE_CRYPT_PROVIDER)
#if defined(__linux__) && defined(_GNU_SOURCE)
    struct crypt_data data;
    std::memset(&data, 0, sizeof(data));
    char* result = ::crypt_r(password.c_str(), salt_or_hash.c_str(), &data);
    if (result == nullptr) {
        throw std::runtime_error("bcrypt crypt_r failed");
    }
    return std::string(result);
#else
    char* result = ::crypt(password.c_str(), salt_or_hash.c_str());
    if (result == nullptr) {
        throw std::runtime_error("bcrypt crypt failed");
    }
    return std::string(result);
#endif
#else
    (void)password;
    (void)salt_or_hash;
    throw std::runtime_error("bcrypt support is unavailable on this platform");
#endif
}

[[nodiscard]] bool is_power_of_two(std::uint64_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] bool openssl_scrypt_available() {
#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
    ByteVector derived(16);
    const ByteVector salt = {'s', 'a', 'l', 't'};
    const std::uint64_t cost = 2;
    const std::uint32_t block_size = 8;
    const std::uint32_t parallelization = 1;
    const std::size_t max_memory = 128ull * block_size * cost * parallelization;

    ERR_clear_error();
    const int ok = EVP_PBE_scrypt("probe",
                                  5,
                                  salt.data(),
                                  salt.size(),
                                  cost,
                                  block_size,
                                  parallelization,
                                  max_memory,
                                  derived.data(),
                                  derived.size());
    ERR_clear_error();
    return ok == 1;
#else
    return false;
#endif
}

} // namespace

std::vector<PasswordHashComparison> compare_password_hashers() {
    return {
        {"PBKDF2-SHA256",
         "Compatibility-first deployments that need broad standards support.",
         "CPU-hard but not memory-hard. Easier to accelerate on GPUs than scrypt or Argon2."},
        {"scrypt",
         "Good memory-hard default when Argon2 is unavailable.",
         "Stronger memory pressure than PBKDF2, but parameter tuning is less ergonomic than Argon2id."},
        {"bcrypt",
         "Legacy systems and ecosystems that already standardize on bcrypt.",
         "Widely supported, but limited input handling and weaker memory hardness than scrypt/Argon2id."},
        {"Argon2id",
         "Modern password storage where memory hardness and side-channel resilience both matter.",
         "Best overall choice when available, but often requires an extra provider/library dependency."}
    };
}

bool supports_pbkdf2_sha256() {
#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
    return true;
#else
    return false;
#endif
}

bool supports_scrypt() {
#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
    static const bool kScryptAvailable = openssl_scrypt_available();
    return kScryptAvailable;
#else
    return false;
#endif
}

bool supports_bcrypt() {
#if defined(COOLBOX_HASH_USE_CRYPT_PROVIDER)
    return true;
#else
    return false;
#endif
}

bool supports_argon2id() {
#if defined(COOLBOX_HASH_USE_ARGON2_PROVIDER)
    return true;
#else
    return false;
#endif
}

ByteVector random_salt(std::size_t length) {
    ByteVector salt(length);
    std::random_device random_device;
    for (auto& byte : salt) {
        byte = static_cast<std::uint8_t>(random_device() & 0xffu);
    }
    return salt;
}

std::string hex_encode(const ByteVector& bytes) {
    std::ostringstream stream;
    stream.setf(std::ios::hex, std::ios::basefield);
    stream.fill('0');
    for (std::uint8_t byte : bytes) {
        stream.width(2);
        stream << static_cast<int>(byte);
    }
    return stream.str();
}

ByteVector pbkdf2_sha256_derive(const std::string& password,
                                const ByteVector& salt,
                                std::uint32_t iterations,
                                std::size_t output_length) {
#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
    if (iterations == 0) {
        throw std::invalid_argument("PBKDF2 iterations must be greater than zero");
    }

    ByteVector derived(output_length);
    const int ok = PKCS5_PBKDF2_HMAC(password.c_str(),
                                     static_cast<int>(password.size()),
                                     salt.data(),
                                     static_cast<int>(salt.size()),
                                     static_cast<int>(iterations),
                                     EVP_sha256(),
                                     static_cast<int>(derived.size()),
                                     derived.data());
    if (ok != 1) {
        throw std::runtime_error("PKCS5_PBKDF2_HMAC failed");
    }
    return derived;
#else
    (void)password;
    (void)salt;
    (void)iterations;
    (void)output_length;
    throw std::runtime_error("PBKDF2-SHA256 requires OpenSSL-compatible EVP support");
#endif
}

std::string pbkdf2_sha256_hash(const std::string& password, const PBKDF2Params& params) {
    const ByteVector salt = random_salt(params.salt_length);
    const ByteVector derived = pbkdf2_sha256_derive(password, salt, params.iterations, params.output_length);
    return to_pbkdf2_encoding(params.iterations, salt, derived);
}

bool verify_pbkdf2_sha256(const std::string& password, const std::string& encoded_hash) {
    const auto parts = split(encoded_hash, '$');
    if (parts.size() != 5 || parts[1] != "coolbox-pbkdf2-sha256") {
        return false;
    }

    const std::uint32_t iterations = parse_u32(parts[2], "iterations");
    const ByteVector salt = base64_decode(parts[3]);
    const ByteVector expected = base64_decode(parts[4]);
    const ByteVector actual = pbkdf2_sha256_derive(password, salt, iterations, expected.size());
    return constant_time_equal(expected, actual);
}

ByteVector scrypt_derive(const std::string& password,
                         const ByteVector& salt,
                         std::uint64_t cost,
                         std::uint32_t block_size,
                         std::uint32_t parallelization,
                         std::size_t output_length) {
#if defined(COOLBOX_HASH_USE_OPENSSL_PROVIDER)
    if (!supports_scrypt()) {
        throw std::runtime_error("scrypt provider unavailable in the linked OpenSSL runtime");
    }

    if (!is_power_of_two(cost) || cost < 2) {
        throw std::invalid_argument("scrypt cost must be a power of two and at least 2");
    }
    if (block_size == 0 || parallelization == 0) {
        throw std::invalid_argument("scrypt block size and parallelization must be greater than zero");
    }

    ByteVector derived(output_length);
    const std::size_t max_memory = 128ull * block_size * cost * parallelization;
    const int ok = EVP_PBE_scrypt(password.c_str(),
                                  password.size(),
                                  salt.data(),
                                  salt.size(),
                                  cost,
                                  block_size,
                                  parallelization,
                                  max_memory,
                                  derived.data(),
                                  derived.size());
    if (ok != 1) {
        throw std::runtime_error("EVP_PBE_scrypt failed");
    }
    return derived;
#else
    (void)password;
    (void)salt;
    (void)cost;
    (void)block_size;
    (void)parallelization;
    (void)output_length;
    throw std::runtime_error("scrypt requires OpenSSL-compatible EVP support");
#endif
}

std::string scrypt_hash(const std::string& password, const ScryptParams& params) {
    const ByteVector salt = random_salt(params.salt_length);
    const ByteVector derived = scrypt_derive(password,
                                             salt,
                                             params.cost,
                                             params.block_size,
                                             params.parallelization,
                                             params.output_length);
    return to_scrypt_encoding(params.cost, params.block_size, params.parallelization, salt, derived);
}

bool verify_scrypt(const std::string& password, const std::string& encoded_hash) {
    const auto parts = split(encoded_hash, '$');
    if (parts.size() != 7 || parts[1] != "coolbox-scrypt") {
        return false;
    }

    const std::uint64_t cost = parse_u64(parts[2], "cost");
    const std::uint32_t block_size = parse_u32(parts[3], "block_size");
    const std::uint32_t parallelization = parse_u32(parts[4], "parallelization");
    const ByteVector salt = base64_decode(parts[5]);
    const ByteVector expected = base64_decode(parts[6]);
    const ByteVector actual = scrypt_derive(password, salt, cost, block_size, parallelization, expected.size());
    return constant_time_equal(expected, actual);
}

std::string bcrypt_hash(const std::string& password, const BCryptParams& params) {
    if (params.cost < 4 || params.cost > 31) {
        throw std::invalid_argument("bcrypt cost must be between 4 and 31");
    }

    ByteVector salt = random_salt(16);
    std::ostringstream salt_prefix;
    salt_prefix << "$2b$";
    if (params.cost < 10) {
        salt_prefix << '0';
    }
    salt_prefix << params.cost << '$';

    std::string encoded_salt = bcrypt_base64_encode(salt);
    if (encoded_salt.size() < 22) {
        encoded_salt.append(22 - encoded_salt.size(), '.');
    }
    salt_prefix << encoded_salt.substr(0, 22);
    return crypt_string(password, salt_prefix.str());
}

bool verify_bcrypt(const std::string& password, const std::string& encoded_hash) {
    if (encoded_hash.rfind("$2", 0) != 0) {
        return false;
    }
    const std::string recomputed = crypt_string(password, encoded_hash);
    return constant_time_equal(recomputed, encoded_hash);
}

std::string argon2id_hash(const std::string& password, const Argon2idParams& params) {
#if defined(COOLBOX_HASH_USE_ARGON2_PROVIDER)
    const ByteVector salt = random_salt(params.salt_length);
    const std::size_t encoded_length = argon2_encodedlen(params.iterations,
                                                         params.memory_kib,
                                                         params.parallelism,
                                                         salt.size(),
                                                         params.output_length,
                                                         Argon2_id);
    std::string encoded(encoded_length, '\0');

    const int status = argon2id_hash_encoded(params.iterations,
                                             params.memory_kib,
                                             params.parallelism,
                                             password.data(),
                                             password.size(),
                                             salt.data(),
                                             salt.size(),
                                             params.output_length,
                                             encoded.data(),
                                             encoded.size());
    if (status != ARGON2_OK) {
        throw std::runtime_error(argon2_error_message(status));
    }

    encoded.resize(std::strlen(encoded.c_str()));
    return encoded;
#else
    (void)password;
    (void)params;
    throw std::runtime_error("Argon2id support requires libargon2 headers and library");
#endif
}

bool verify_argon2id(const std::string& password, const std::string& encoded_hash) {
#if defined(COOLBOX_HASH_USE_ARGON2_PROVIDER)
    return argon2id_verify(encoded_hash.c_str(), password.data(), password.size()) == ARGON2_OK;
#else
    (void)password;
    (void)encoded_hash;
    throw std::runtime_error("Argon2id support requires libargon2 headers and library");
#endif
}

} // namespace hash
} // namespace utils
