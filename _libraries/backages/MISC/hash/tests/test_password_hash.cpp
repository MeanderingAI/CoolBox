#include "tyst_framework.hpp"

#include "password_hash.hpp"

namespace {

utils::hash::ByteVector make_bytes(std::initializer_list<std::uint8_t> values) {
    return utils::hash::ByteVector(values);
}

} // namespace

TEST(PasswordHashComparisonTest, IncludesAllRequestedAlgorithms) {
    const auto comparisons = utils::hash::compare_password_hashers();
    ASSERT_EQ(comparisons.size(), 4u);
    EXPECT_EQ(comparisons[0].name, "PBKDF2-SHA256");
    EXPECT_EQ(comparisons[1].name, "scrypt");
    EXPECT_EQ(comparisons[2].name, "bcrypt");
    EXPECT_EQ(comparisons[3].name, "Argon2id");
}

TEST(PasswordHashUtilityTest, RandomSaltReturnsRequestedLength) {
    const auto salt = utils::hash::random_salt(24);
    EXPECT_EQ(salt.size(), 24u);
}

TEST(PasswordHashUtilityTest, HexEncodeProducesLowercaseHex) {
    EXPECT_EQ(utils::hash::hex_encode(make_bytes({0x12, 0xab, 0x00, 0xff})), "12ab00ff");
}

TEST(PBKDF2PasswordHashTest, SupportsAvailabilityQuery) {
    if (!utils::hash::supports_pbkdf2_sha256()) {
        TYST_SKIP() << "PBKDF2 provider unavailable in this build";
    }

    const auto derived = utils::hash::pbkdf2_sha256_derive("password",
                                                            make_bytes({'s', 'a', 'l', 't'}),
                                                            1,
                                                            32);
    EXPECT_EQ(utils::hash::hex_encode(derived),
              "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
}

TEST(PBKDF2PasswordHashTest, EncodedHashRoundTrips) {
    if (!utils::hash::supports_pbkdf2_sha256()) {
        TYST_SKIP() << "PBKDF2 provider unavailable in this build";
    }

    utils::hash::PBKDF2Params params;
    params.iterations = 1000;
    params.output_length = 32;
    params.salt_length = 16;

    const std::string encoded = utils::hash::pbkdf2_sha256_hash("correct horse battery staple", params);
    EXPECT_TRUE(utils::hash::verify_pbkdf2_sha256("correct horse battery staple", encoded));
    EXPECT_FALSE(utils::hash::verify_pbkdf2_sha256("wrong password", encoded));
}

TEST(ScryptPasswordHashTest, SupportsAvailabilityQuery) {
    if (!utils::hash::supports_scrypt()) {
        TYST_SKIP() << "scrypt provider unavailable in this build";
    }

    const auto derived = utils::hash::scrypt_derive("",
                                                     {},
                                                     16,
                                                     1,
                                                     1,
                                                     64);
    EXPECT_EQ(utils::hash::hex_encode(derived),
              "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442"
              "fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906");
}

TEST(ScryptPasswordHashTest, EncodedHashRoundTrips) {
    if (!utils::hash::supports_scrypt()) {
        TYST_SKIP() << "scrypt provider unavailable in this build";
    }

    utils::hash::ScryptParams params;
    params.cost = 1u << 14;
    params.block_size = 8;
    params.parallelization = 1;
    params.output_length = 32;
    params.salt_length = 16;

    const std::string encoded = utils::hash::scrypt_hash("hunter2", params);
    EXPECT_TRUE(utils::hash::verify_scrypt("hunter2", encoded));
    EXPECT_FALSE(utils::hash::verify_scrypt("wrong", encoded));
}

TEST(BCryptPasswordHashTest, RoundTripsWhenSupported) {
    if (!utils::hash::supports_bcrypt()) {
        TYST_SKIP() << "bcrypt provider unavailable in this build";
    }

    utils::hash::BCryptParams params;
    params.cost = 10;

    const std::string encoded = utils::hash::bcrypt_hash("swordfish", params);
    EXPECT_TRUE(utils::hash::verify_bcrypt("swordfish", encoded));
    EXPECT_FALSE(utils::hash::verify_bcrypt("Swordfish", encoded));
}

TEST(Argon2PasswordHashTest, RoundTripsWhenSupported) {
    if (!utils::hash::supports_argon2id()) {
        TYST_SKIP() << "Argon2id provider unavailable in this build";
    }

    utils::hash::Argon2idParams params;
    params.iterations = 2;
    params.memory_kib = 32768;
    params.parallelism = 1;
    params.output_length = 32;
    params.salt_length = 16;

    const std::string encoded = utils::hash::argon2id_hash("letmein", params);
    EXPECT_TRUE(utils::hash::verify_argon2id("letmein", encoded));
    EXPECT_FALSE(utils::hash::verify_argon2id("wrong", encoded));
}
