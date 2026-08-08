#include "custom_numbers.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    using security::custom_numbers::BigUnsigned;
    using security::custom_numbers::gcd;
    using security::custom_numbers::mod_pow;

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