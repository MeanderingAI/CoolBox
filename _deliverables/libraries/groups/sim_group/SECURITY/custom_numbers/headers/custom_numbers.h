#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace security::custom_numbers {

// These profiles mirror the generations compared as dmg1997 and dmg2017 in
// research.swtch.com/fp. They use standard-library facilities, not dtoa.c.
enum class FloatingPointConversion {
    Automatic,
    Dmg1997,
    Dmg2017,
    Dblconv,
    Abseil,
    Uscale,
    FastFloat,
    Libc,
    UscaleC
};

struct FloatingPointConversionCapabilities {
    bool format;
    bool parse;
};

FloatingPointConversionCapabilities floating_point_conversion_capabilities(
    FloatingPointConversion conversion);
bool floating_point_conversion_available(FloatingPointConversion conversion);
FloatingPointConversion selected_floating_point_conversion(
    FloatingPointConversion requested = FloatingPointConversion::Automatic);
const char* floating_point_conversion_name(FloatingPointConversion conversion);

std::string format_double(
    double value,
    FloatingPointConversion conversion = FloatingPointConversion::Automatic);
bool parse_double(
    std::string_view text,
    double& value,
    FloatingPointConversion conversion = FloatingPointConversion::Automatic);

class BigUnsigned;

struct DivisionResult {
    BigUnsigned* quotient;
    BigUnsigned* remainder;
};

class BigUnsigned {
public:
    BigUnsigned();
    BigUnsigned(std::uint64_t value);

    static BigUnsigned from_decimal(const std::string& value);
    static BigUnsigned from_hex(const std::string& value);

    bool is_zero() const;
    std::size_t bit_length() const;
    bool test_bit(std::size_t bit_index) const;

    std::string to_decimal_string() const;
    std::string to_hex_string() const;

    int compare(const BigUnsigned& other) const;

    BigUnsigned& operator+=(const BigUnsigned& other);
    BigUnsigned& operator-=(const BigUnsigned& other);
    BigUnsigned& operator*=(const BigUnsigned& other);
    BigUnsigned& operator/=(const BigUnsigned& other);
    BigUnsigned& operator%=(const BigUnsigned& other);

private:
    std::vector<std::uint32_t> limbs_;

    void normalize();
    void shift_left_one();
    void set_bit(std::size_t bit_index);
    void multiply_by_uint32(std::uint32_t value);
    void add_uint32(std::uint32_t value);
    std::uint32_t divide_by_uint32(std::uint32_t value);

    friend void div_mod(const BigUnsigned& dividend,
                        const BigUnsigned& divisor,
                        BigUnsigned& quotient,
                        BigUnsigned& remainder);
};

bool operator==(const BigUnsigned& left, const BigUnsigned& right);
bool operator!=(const BigUnsigned& left, const BigUnsigned& right);
bool operator<(const BigUnsigned& left, const BigUnsigned& right);
bool operator<=(const BigUnsigned& left, const BigUnsigned& right);
bool operator>(const BigUnsigned& left, const BigUnsigned& right);
bool operator>=(const BigUnsigned& left, const BigUnsigned& right);

BigUnsigned operator+(BigUnsigned left, const BigUnsigned& right);
BigUnsigned operator-(BigUnsigned left, const BigUnsigned& right);
BigUnsigned operator*(BigUnsigned left, const BigUnsigned& right);
BigUnsigned operator/(BigUnsigned left, const BigUnsigned& right);
BigUnsigned operator%(BigUnsigned left, const BigUnsigned& right);

void div_mod(const BigUnsigned& dividend,
             const BigUnsigned& divisor,
             BigUnsigned& quotient,
             BigUnsigned& remainder);

BigUnsigned gcd(BigUnsigned left, BigUnsigned right);
BigUnsigned mod_pow(BigUnsigned base, const BigUnsigned& exponent, const BigUnsigned& modulus);

} // namespace security::custom_numbers