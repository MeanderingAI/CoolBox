#include "tyst_framework.hpp"

#include "uuid_generation.hpp"

#include <cctype>
#include <string>
#include <vector>

using trekker::misc::uuid_generation::Uuid;
using namespace trekker::misc::uuid_generation;

namespace {

int extract_version(const Uuid& id) {
    const std::string value = id.to_string();
    return std::isdigit(static_cast<unsigned char>(value[14]))
        ? (value[14] - '0')
        : (10 + (std::tolower(static_cast<unsigned char>(value[14])) - 'a'));
}

bool is_rfc4122_variant(const Uuid& id) {
    const std::string value = id.to_string();
    const char variant = static_cast<char>(std::tolower(static_cast<unsigned char>(value[19])));
    return variant == '8' || variant == '9' || variant == 'a' || variant == 'b';
}

} // namespace

TEST(UuidGenerationTest, GeneratesV1) {
    const Uuid id = generate_v1();
    EXPECT_EQ(extract_version(id), 1);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesV2) {
    const Uuid id = generate_v2(0x12345678U, 0x02U);
    EXPECT_EQ(extract_version(id), 2);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesV3Deterministically) {
    const Uuid ns = namespaces::dns();
    const Uuid id1 = generate_v3(ns, "example.com");
    const Uuid id2 = generate_v3(ns, "example.com");
    EXPECT_EQ(id1.to_string(), id2.to_string());
    EXPECT_EQ(extract_version(id1), 3);
}

TEST(UuidGenerationTest, GeneratesV4) {
    const Uuid id = generate_v4();
    EXPECT_EQ(extract_version(id), 4);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesV5Deterministically) {
    const Uuid ns = namespaces::url();
    const Uuid id1 = generate_v5(ns, "https://coolbox.dev");
    const Uuid id2 = generate_v5(ns, "https://coolbox.dev");
    EXPECT_EQ(id1.to_string(), id2.to_string());
    EXPECT_EQ(extract_version(id1), 5);
}

TEST(UuidGenerationTest, GeneratesV6) {
    const Uuid id = generate_v6();
    EXPECT_EQ(extract_version(id), 6);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesV7) {
    const Uuid id = generate_v7();
    EXPECT_EQ(extract_version(id), 7);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesV8) {
    const std::vector<std::uint8_t> entropy(16, 0xAB);
    const Uuid id = generate_v8(entropy);
    EXPECT_EQ(extract_version(id), 8);
    EXPECT_TRUE(is_rfc4122_variant(id));
}

TEST(UuidGenerationTest, GeneratesGuidString) {
    const std::string guid = generate_guid();
    EXPECT_EQ(guid.size(), 38U);
    EXPECT_EQ(guid.front(), '{');
    EXPECT_EQ(guid.back(), '}');
}

TEST(UuidGenerationTest, UuidStringHasCanonicalLayout) {
    const std::string value = generate_v4().to_string();
    EXPECT_EQ(value.size(), 36U);
    EXPECT_EQ(value[8], '-');
    EXPECT_EQ(value[13], '-');
    EXPECT_EQ(value[18], '-');
    EXPECT_EQ(value[23], '-');
}
