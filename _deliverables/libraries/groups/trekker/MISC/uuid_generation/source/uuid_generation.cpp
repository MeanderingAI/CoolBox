#include "uuid_generation.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(COOLBOX_UUID_USE_OPENSSL)
#include <openssl/md5.h>
#include <openssl/sha.h>
#endif

namespace trekker {
namespace misc {
namespace uuid_generation {
namespace {

constexpr std::uint64_t UUID_EPOCH_OFFSET_100NS = 0x01B21DD213814000ULL;

std::mt19937_64& rng() {
    static std::mt19937_64 engine{std::random_device{}()};
    return engine;
}

std::uint64_t random_u64() {
    return rng()();
}

std::uint16_t random_clock_seq() {
    return static_cast<std::uint16_t>(random_u64() & 0x3fffU);
}

std::array<std::uint8_t, 6> random_node_id() {
    std::array<std::uint8_t, 6> node{};
    std::uint64_t value = random_u64();
    for (std::size_t i = 0; i < 6; ++i) {
        node[5 - i] = static_cast<std::uint8_t>(value & 0xffU);
        value >>= 8U;
    }
    // Mark as locally administered + unicast-style random node.
    node[0] |= 0x01U;
    return node;
}

std::uint64_t now_100ns_since_gregorian_epoch() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto unix_100ns = std::chrono::duration_cast<std::chrono::duration<std::uint64_t, std::ratio<1, 10000000>>>(now).count();
    return unix_100ns + UUID_EPOCH_OFFSET_100NS;
}

void set_variant_rfc4122(Uuid& id) {
    id.bytes[8] = static_cast<std::uint8_t>((id.bytes[8] & 0x3fU) | 0x80U);
}

void set_version(Uuid& id, std::uint8_t version) {
    id.bytes[6] = static_cast<std::uint8_t>((id.bytes[6] & 0x0fU) | (version << 4U));
}

std::vector<std::uint8_t> namespace_and_name_bytes(const Uuid& ns, const std::string& name) {
    std::vector<std::uint8_t> data;
    data.reserve(16 + name.size());
    data.insert(data.end(), ns.bytes.begin(), ns.bytes.end());
    data.insert(data.end(), name.begin(), name.end());
    return data;
}

std::array<std::uint8_t, 16> fallback_hash_128(const std::vector<std::uint8_t>& data, std::uint64_t seed) {
    // Lightweight deterministic fallback used only when OpenSSL is unavailable.
    std::uint64_t h1 = 1469598103934665603ULL ^ seed;
    std::uint64_t h2 = 1099511628211ULL ^ (seed << 1U);

    for (std::uint8_t b : data) {
        h1 ^= b;
        h1 *= 1099511628211ULL;
        h2 += static_cast<std::uint64_t>(b) + 0x9e3779b97f4a7c15ULL;
        h2 = (h2 ^ (h2 >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        h2 = (h2 ^ (h2 >> 27U)) * 0x94d049bb133111ebULL;
        h2 ^= (h2 >> 31U);
    }

    std::array<std::uint8_t, 16> out{};
    for (int i = 0; i < 8; ++i) {
        out[i] = static_cast<std::uint8_t>((h1 >> ((7 - i) * 8)) & 0xffU);
        out[8 + i] = static_cast<std::uint8_t>((h2 >> ((7 - i) * 8)) & 0xffU);
    }
    return out;
}

std::array<std::uint8_t, 16> md5_like(const std::vector<std::uint8_t>& data) {
#if defined(COOLBOX_UUID_USE_OPENSSL)
    std::array<std::uint8_t, 16> out{};
    MD5(data.data(), data.size(), out.data());
    return out;
#else
    return fallback_hash_128(data, 0x4d44352ULL);
#endif
}

std::array<std::uint8_t, 16> sha1_like_128(const std::vector<std::uint8_t>& data) {
#if defined(COOLBOX_UUID_USE_OPENSSL)
    std::array<std::uint8_t, SHA_DIGEST_LENGTH> digest{};
    SHA1(data.data(), data.size(), digest.data());
    std::array<std::uint8_t, 16> out{};
    std::copy_n(digest.begin(), 16, out.begin());
    return out;
#else
    return fallback_hash_128(data, 0x53484131ULL);
#endif
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

Uuid from_timestamp_and_node(std::uint64_t timestamp_100ns, std::uint16_t clock_seq, const std::array<std::uint8_t, 6>& node, std::uint8_t version) {
    Uuid out{};

    const std::uint32_t time_low = static_cast<std::uint32_t>(timestamp_100ns & 0xffffffffULL);
    const std::uint16_t time_mid = static_cast<std::uint16_t>((timestamp_100ns >> 32U) & 0xffffULL);
    const std::uint16_t time_hi = static_cast<std::uint16_t>((timestamp_100ns >> 48U) & 0x0fffULL);

    out.bytes[0] = static_cast<std::uint8_t>((time_low >> 24U) & 0xffU);
    out.bytes[1] = static_cast<std::uint8_t>((time_low >> 16U) & 0xffU);
    out.bytes[2] = static_cast<std::uint8_t>((time_low >> 8U) & 0xffU);
    out.bytes[3] = static_cast<std::uint8_t>(time_low & 0xffU);

    out.bytes[4] = static_cast<std::uint8_t>((time_mid >> 8U) & 0xffU);
    out.bytes[5] = static_cast<std::uint8_t>(time_mid & 0xffU);

    out.bytes[6] = static_cast<std::uint8_t>((time_hi >> 8U) & 0x0fU);
    out.bytes[7] = static_cast<std::uint8_t>(time_hi & 0xffU);

    out.bytes[8] = static_cast<std::uint8_t>((clock_seq >> 8U) & 0x3fU);
    out.bytes[9] = static_cast<std::uint8_t>(clock_seq & 0xffU);

    for (std::size_t i = 0; i < 6; ++i) {
        out.bytes[10 + i] = node[i];
    }

    set_variant_rfc4122(out);
    set_version(out, version);
    return out;
}

} // namespace

std::string Uuid::to_string(bool uppercase) const {
    std::ostringstream oss;
    if (uppercase) {
        oss.setf(std::ios::uppercase);
    }
    oss << std::hex << std::setfill('0')
        << std::setw(2) << static_cast<int>(bytes[0])
        << std::setw(2) << static_cast<int>(bytes[1])
        << std::setw(2) << static_cast<int>(bytes[2])
        << std::setw(2) << static_cast<int>(bytes[3])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[4])
        << std::setw(2) << static_cast<int>(bytes[5])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[6])
        << std::setw(2) << static_cast<int>(bytes[7])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[8])
        << std::setw(2) << static_cast<int>(bytes[9])
        << "-"
        << std::setw(2) << static_cast<int>(bytes[10])
        << std::setw(2) << static_cast<int>(bytes[11])
        << std::setw(2) << static_cast<int>(bytes[12])
        << std::setw(2) << static_cast<int>(bytes[13])
        << std::setw(2) << static_cast<int>(bytes[14])
        << std::setw(2) << static_cast<int>(bytes[15]);
    return oss.str();
}

std::string Uuid::to_guid_string(bool uppercase) const {
    const std::string body = to_string(uppercase);
    return "{" + body + "}";
}

Uuid parse_uuid(const std::string& value) {
    std::string compact;
    compact.reserve(32);

    for (char c : value) {
        if (c == '-' || c == '{' || c == '}') {
            continue;
        }
        compact.push_back(c);
    }

    if (compact.size() != 32) {
        throw std::invalid_argument("UUID parse failed: expected 32 hex characters");
    }

    Uuid out{};
    for (std::size_t i = 0; i < 16; ++i) {
        const int hi = hex_to_int(compact[i * 2]);
        const int lo = hex_to_int(compact[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            throw std::invalid_argument("UUID parse failed: invalid hex character");
        }
        out.bytes[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }

    return out;
}

namespace namespaces {

Uuid dns() {
    return parse_uuid("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
}

Uuid url() {
    return parse_uuid("6ba7b811-9dad-11d1-80b4-00c04fd430c8");
}

Uuid oid() {
    return parse_uuid("6ba7b812-9dad-11d1-80b4-00c04fd430c8");
}

Uuid x500() {
    return parse_uuid("6ba7b814-9dad-11d1-80b4-00c04fd430c8");
}

} // namespace namespaces

Uuid generate_v1() {
    static std::atomic<std::uint64_t> last_timestamp{0};
    static const auto node = random_node_id();
    static const std::uint16_t clock_seq = random_clock_seq();

    std::uint64_t ts = now_100ns_since_gregorian_epoch();
    std::uint64_t prev = last_timestamp.load();
    while (ts <= prev && !last_timestamp.compare_exchange_weak(prev, prev + 1)) {
    }
    if (ts <= prev) {
        ts = prev + 1;
    }
    last_timestamp.store(ts);

    return from_timestamp_and_node(ts, clock_seq, node, 1);
}

Uuid generate_v2(std::uint32_t local_identifier, std::uint8_t local_domain) {
    Uuid out = generate_v1();

    // DCE Security UUID variant: overwrite low field with local identifier and
    // place local domain in the low clock sequence byte.
    out.bytes[0] = static_cast<std::uint8_t>((local_identifier >> 24U) & 0xffU);
    out.bytes[1] = static_cast<std::uint8_t>((local_identifier >> 16U) & 0xffU);
    out.bytes[2] = static_cast<std::uint8_t>((local_identifier >> 8U) & 0xffU);
    out.bytes[3] = static_cast<std::uint8_t>(local_identifier & 0xffU);
    out.bytes[9] = local_domain;

    set_version(out, 2);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v3(const Uuid& namespace_uuid, const std::string& name) {
    Uuid out{};
    out.bytes = md5_like(namespace_and_name_bytes(namespace_uuid, name));
    set_version(out, 3);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v4() {
    Uuid out{};
    for (std::size_t i = 0; i < 16; i += 8) {
        std::uint64_t value = random_u64();
        for (std::size_t b = 0; b < 8; ++b) {
            out.bytes[i + (7 - b)] = static_cast<std::uint8_t>(value & 0xffU);
            value >>= 8U;
        }
    }
    set_version(out, 4);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v5(const Uuid& namespace_uuid, const std::string& name) {
    Uuid out{};
    out.bytes = sha1_like_128(namespace_and_name_bytes(namespace_uuid, name));
    set_version(out, 5);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v6() {
    static std::atomic<std::uint64_t> last_timestamp{0};
    static const auto node = random_node_id();
    static const std::uint16_t clock_seq = random_clock_seq();

    std::uint64_t ts = now_100ns_since_gregorian_epoch();
    std::uint64_t prev = last_timestamp.load();
    while (ts <= prev && !last_timestamp.compare_exchange_weak(prev, prev + 1)) {
    }
    if (ts <= prev) {
        ts = prev + 1;
    }
    last_timestamp.store(ts);

    Uuid out{};

    // UUIDv6 packs the timestamp in lexicographically sortable order.
    out.bytes[0] = static_cast<std::uint8_t>((ts >> 52U) & 0xffU);
    out.bytes[1] = static_cast<std::uint8_t>((ts >> 44U) & 0xffU);
    out.bytes[2] = static_cast<std::uint8_t>((ts >> 36U) & 0xffU);
    out.bytes[3] = static_cast<std::uint8_t>((ts >> 28U) & 0xffU);
    out.bytes[4] = static_cast<std::uint8_t>((ts >> 20U) & 0xffU);
    out.bytes[5] = static_cast<std::uint8_t>((ts >> 12U) & 0xffU);
    out.bytes[6] = static_cast<std::uint8_t>((ts >> 8U) & 0x0fU);
    out.bytes[7] = static_cast<std::uint8_t>(ts & 0xffU);

    out.bytes[8] = static_cast<std::uint8_t>((clock_seq >> 8U) & 0x3fU);
    out.bytes[9] = static_cast<std::uint8_t>(clock_seq & 0xffU);

    for (std::size_t i = 0; i < 6; ++i) {
        out.bytes[10 + i] = node[i];
    }

    set_version(out, 6);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v7() {
    Uuid out = generate_v4();

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const std::uint64_t unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    // UUIDv7: first 48 bits are big-endian Unix epoch milliseconds.
    out.bytes[0] = static_cast<std::uint8_t>((unix_ms >> 40U) & 0xffU);
    out.bytes[1] = static_cast<std::uint8_t>((unix_ms >> 32U) & 0xffU);
    out.bytes[2] = static_cast<std::uint8_t>((unix_ms >> 24U) & 0xffU);
    out.bytes[3] = static_cast<std::uint8_t>((unix_ms >> 16U) & 0xffU);
    out.bytes[4] = static_cast<std::uint8_t>((unix_ms >> 8U) & 0xffU);
    out.bytes[5] = static_cast<std::uint8_t>(unix_ms & 0xffU);

    set_version(out, 7);
    set_variant_rfc4122(out);
    return out;
}

Uuid generate_v8(const std::vector<std::uint8_t>& custom_entropy) {
    Uuid out{};

    std::vector<std::uint8_t> entropy = custom_entropy;
    if (entropy.size() < 16) {
        entropy.resize(16, 0);
        const auto rnd = generate_v4();
        for (std::size_t i = 0; i < 16; ++i) {
            entropy[i] ^= rnd.bytes[i];
        }
    }

    std::copy_n(entropy.begin(), 16, out.bytes.begin());
    set_version(out, 8);
    set_variant_rfc4122(out);
    return out;
}

std::string generate_guid() {
    return generate_v4().to_guid_string(true);
}

} // namespace uuid_generation
} // namespace misc
} // namespace trekker
