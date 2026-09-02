#include "custom_numbers.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

std::uint64_t double_bits(double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void test_floating_point_conversion() {
    using security::custom_numbers::FloatingPointConversion;
    using security::custom_numbers::format_double;
    using security::custom_numbers::parse_double;

    const double values[] = {
        0.0,
        -0.0,
        0.1,
        -123456.75,
        std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max()
    };
    const FloatingPointConversion conversions[] = {
        FloatingPointConversion::Automatic,
        FloatingPointConversion::Dmg1997,
        FloatingPointConversion::Dmg2017,
        FloatingPointConversion::Dblconv,
        FloatingPointConversion::Abseil,
        FloatingPointConversion::Uscale,
        FloatingPointConversion::FastFloat,
        FloatingPointConversion::Libc,
        FloatingPointConversion::UscaleC
    };

    for (const auto conversion : conversions) {
        for (double expected : values) {
            const std::string text = format_double(expected, conversion);
            double actual = 0.0;
            assert(parse_double(text, actual, conversion));
            assert(double_bits(expected) == double_bits(actual));
        }
    }

    double unchanged = 42.0;
    assert(!parse_double("1.25 trailing", unchanged));
    assert(unchanged == 42.0);

    const auto fast_float_capabilities =
        security::custom_numbers::floating_point_conversion_capabilities(
            FloatingPointConversion::FastFloat);
    assert(!fast_float_capabilities.format);
    const auto uscale_capabilities =
        security::custom_numbers::floating_point_conversion_capabilities(
            FloatingPointConversion::Uscale);
    assert(!uscale_capabilities.format && !uscale_capabilities.parse);
}

} // namespace

int main() {
    using security::custom_numbers::BigUnsigned;
    using security::custom_numbers::gcd;
    using security::custom_numbers::mod_pow;

    test_floating_point_conversion();

    const BigUnsigned large = BigUnsigned::from_decimal("123456789012345678901234567890");
    assert(large.to_decimal_string() == "123456789012345678901234567890");
    assert(BigUnsigned::from_hex("0x1234abcd").to_hex_string() == "0x1234abcd");

    const BigUnsigned left = BigUnsigned::from_decimal("12345678901234567890");
    const BigUnsigned right = BigUnsigned::from_decimal("98765432109876543210");
    assert((left + right).to_decimal_string() == "111111111011111111100");
    assert((right - left).to_decimal_string() == "86419753208641975320");

    const BigUnsigned multiplicand = BigUnsigned::from_decimal("123456789");
    const BigUnsigned multiplier = BigUnsigned::from_decimal("987654321");
    assert((multiplicand * multiplier).to_decimal_string() == "121932631112635269");

    const BigUnsigned dividend = BigUnsigned::from_decimal("121932631112635269");
    assert((dividend / multiplicand).to_decimal_string() == "987654321");
    assert((dividend % multiplicand).to_decimal_string() == "0");

    assert(gcd(BigUnsigned::from_decimal("391"), BigUnsigned::from_decimal("299")).to_decimal_string() == "23");
    assert(mod_pow(BigUnsigned(4), BigUnsigned(13), BigUnsigned(497)).to_decimal_string() == "445");
    assert(mod_pow(BigUnsigned::from_decimal("12345678901234567890"),
                   BigUnsigned(17),
                   BigUnsigned::from_decimal("1000000007")).to_decimal_string() == "920208758");

    bool rejected_negative_subtraction = false;
    try {
        BigUnsigned(1) - BigUnsigned(2);
    } catch (const std::underflow_error&) {
        rejected_negative_subtraction = true;
    }
    assert(rejected_negative_subtraction);

    std::cout << "custom_numbers tests passed\n";
    return 0;
}