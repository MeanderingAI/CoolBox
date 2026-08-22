#include "../headers/custom_numbers.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

#ifdef CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION
#include <double-conversion/double-conversion.h>
#endif

#ifdef CUSTOM_NUMBERS_HAS_ABSEIL
#include <absl/strings/charconv.h>
#endif

#ifdef CUSTOM_NUMBERS_HAS_FAST_FLOAT
#include <fast_float/fast_float.h>
#endif

namespace security::custom_numbers {

namespace {

constexpr std::uint64_t kLimbBase = 1ull << 32u;

std::uint8_t hex_value(char value) {
    if (value >= '0' && value <= '9') return static_cast<std::uint8_t>(value - '0');
    if (value >= 'a' && value <= 'f') return static_cast<std::uint8_t>(value - 'a' + 10);
    if (value >= 'A' && value <= 'F') return static_cast<std::uint8_t>(value - 'A' + 10);
    throw std::invalid_argument("invalid hexadecimal digit");
}

std::string format_double_portable(double value) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return output.str();
}

bool parse_double_portable(std::string_view text, double& value) {
    if (text.empty() || std::any_of(text.begin(), text.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        })) {
        return false;
    }

    const std::string input(text);
    char* end = nullptr;
    const double parsed = std::strtod(input.c_str(), &end);
    if (end == input.c_str() || end != input.c_str() + input.size()) {
        return false;
    }
    value = parsed;
    return true;
}

std::string format_double_libc(double value) {
    char buffer[64];
    const int length = std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    if (length < 0 || static_cast<std::size_t>(length) >= sizeof(buffer)) {
        throw std::runtime_error("libc failed to format double");
    }
    return std::string(buffer, static_cast<std::size_t>(length));
}

#ifdef CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION
std::string format_double_dblconv(double value) {
    char buffer[128];
    double_conversion::StringBuilder builder(buffer, sizeof(buffer));
    const double_conversion::DoubleToStringConverter converter(
        double_conversion::DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN,
        "Infinity",
        "NaN",
        'e',
        -6,
        21,
        6,
        0);
    if (!converter.ToShortest(value, &builder)) {
        throw std::runtime_error("double-conversion failed to format double");
    }
    return builder.Finalize();
}

bool parse_double_dblconv(std::string_view text, double& value) {
    const double_conversion::StringToDoubleConverter converter(
        double_conversion::StringToDoubleConverter::NO_FLAGS,
        0.0,
        std::numeric_limits<double>::quiet_NaN(),
        "Infinity",
        "NaN");
    int processed = 0;
    const double parsed = converter.StringToDouble(
        text.data(), static_cast<int>(text.size()), &processed);
    if (processed != static_cast<int>(text.size()) || text.empty()) {
        return false;
    }
    value = parsed;
    return true;
}
#endif

} // namespace

FloatingPointConversionCapabilities floating_point_conversion_capabilities(
    FloatingPointConversion conversion) {
    switch (conversion) {
        case FloatingPointConversion::Automatic:
        case FloatingPointConversion::Dmg1997:
        case FloatingPointConversion::Libc:
            return {true, true};
        case FloatingPointConversion::Dmg2017:
#ifdef CUSTOM_NUMBERS_HAS_FLOAT_CHARCONV
            return {true, true};
#else
            return {false, false};
#endif
        case FloatingPointConversion::Dblconv:
#ifdef CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION
            return {true, true};
#else
            return {false, false};
#endif
        case FloatingPointConversion::Abseil:
#ifdef CUSTOM_NUMBERS_HAS_ABSEIL
            return {false, true};
#else
            return {false, false};
#endif
        case FloatingPointConversion::FastFloat:
#ifdef CUSTOM_NUMBERS_HAS_FAST_FLOAT
            return {false, true};
#else
            return {false, false};
#endif
        case FloatingPointConversion::Uscale:
        case FloatingPointConversion::UscaleC:
            return {false, false};
        default:
            return {false, false};
    }
}

bool floating_point_conversion_available(FloatingPointConversion conversion) {
    const auto capabilities = floating_point_conversion_capabilities(conversion);
    return capabilities.format && capabilities.parse;
}

FloatingPointConversion selected_floating_point_conversion(FloatingPointConversion requested) {
    if (requested == FloatingPointConversion::Automatic) {
        return floating_point_conversion_available(FloatingPointConversion::Dmg2017)
                   ? FloatingPointConversion::Dmg2017
                   : FloatingPointConversion::Dmg1997;
    }
    return floating_point_conversion_available(requested)
               ? requested
               : FloatingPointConversion::Dmg1997;
}

const char* floating_point_conversion_name(FloatingPointConversion conversion) {
    switch (conversion) {
        case FloatingPointConversion::Automatic: return "automatic";
        case FloatingPointConversion::Dmg1997: return "dmg1997_profile";
        case FloatingPointConversion::Dmg2017: return "dmg2017_profile";
        case FloatingPointConversion::Dblconv: return "dblconv";
        case FloatingPointConversion::Abseil: return "abseil";
        case FloatingPointConversion::Uscale: return "uscale";
        case FloatingPointConversion::FastFloat: return "fast_float";
        case FloatingPointConversion::Libc: return "libc";
        case FloatingPointConversion::UscaleC: return "uscalec";
        default: return "unknown";
    }
}

std::string format_double(double value, FloatingPointConversion conversion) {
    const auto requested = conversion == FloatingPointConversion::Automatic
                               ? selected_floating_point_conversion(conversion)
                               : conversion;

#ifdef CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION
    if (requested == FloatingPointConversion::Dblconv) {
        return format_double_dblconv(value);
    }
#endif

    if (requested == FloatingPointConversion::Libc) {
        return format_double_libc(value);
    }

#ifdef CUSTOM_NUMBERS_HAS_FLOAT_CHARCONV
    if (requested == FloatingPointConversion::Dmg2017) {
        char buffer[64];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (result.ec == std::errc{}) {
            return std::string(buffer, result.ptr);
        }
    }
#else
    (void)conversion;
#endif
    return format_double_portable(value);
}

bool parse_double(std::string_view text, double& value, FloatingPointConversion conversion) {
    const auto requested = conversion == FloatingPointConversion::Automatic
                               ? selected_floating_point_conversion(conversion)
                               : conversion;

#ifdef CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION
    if (requested == FloatingPointConversion::Dblconv) {
        return parse_double_dblconv(text, value);
    }
#endif

#ifdef CUSTOM_NUMBERS_HAS_ABSEIL
    if (requested == FloatingPointConversion::Abseil) {
        double parsed = 0.0;
        const auto result = absl::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
            value = parsed;
            return true;
        }
        return false;
    }
#endif

#ifdef CUSTOM_NUMBERS_HAS_FAST_FLOAT
    if (requested == FloatingPointConversion::FastFloat) {
        double parsed = 0.0;
        const auto result = fast_float::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
            value = parsed;
            return true;
        }
        return false;
    }
#endif

#ifdef CUSTOM_NUMBERS_HAS_FLOAT_CHARCONV
    if (requested == FloatingPointConversion::Dmg2017) {
        double parsed = 0.0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
            value = parsed;
            return true;
        }
        return false;
    }
#else
    (void)conversion;
#endif
    return parse_double_portable(text, value);
}

BigUnsigned::BigUnsigned() = default;

BigUnsigned::BigUnsigned(std::uint64_t value) {
    if (value == 0) {
        return;
    }
    limbs_.push_back(static_cast<std::uint32_t>(value & 0xffffffffu));
    const std::uint32_t high = static_cast<std::uint32_t>(value >> 32u);
    if (high != 0) {
        limbs_.push_back(high);
    }
}

BigUnsigned BigUnsigned::from_decimal(const std::string& value) {
    BigUnsigned result;
    bool saw_digit = false;

    for (unsigned char ch : value) {
        if (std::isspace(ch)) {
            continue;
        }
        if (!std::isdigit(ch)) {
            throw std::invalid_argument("decimal integer contains a non-digit character");
        }
        saw_digit = true;
        result.multiply_by_uint32(10);
        result.add_uint32(static_cast<std::uint32_t>(ch - '0'));
    }

    if (!saw_digit) {
        throw std::invalid_argument("decimal integer is empty");
    }
    return result;
}

BigUnsigned BigUnsigned::from_hex(const std::string& value) {
    std::size_t index = 0;
    while (index < value.size() && std::isspace(static_cast<unsigned char>(value[index]))) {
        ++index;
    }
    if (index + 1 < value.size() && value[index] == '0' && (value[index + 1] == 'x' || value[index + 1] == 'X')) {
        index += 2;
    }

    BigUnsigned result;
    bool saw_digit = false;
    for (; index < value.size(); ++index) {
        if (std::isspace(static_cast<unsigned char>(value[index]))) {
            continue;
        }
        saw_digit = true;
        result.multiply_by_uint32(16);
        result.add_uint32(hex_value(value[index]));
    }

    if (!saw_digit) {
        throw std::invalid_argument("hex integer is empty");
    }
    return result;
}

bool BigUnsigned::is_zero() const {
    return limbs_.empty();
}

std::size_t BigUnsigned::bit_length() const {
    if (limbs_.empty()) {
        return 0;
    }

    const std::uint32_t top = limbs_.back();
    std::size_t bits = (limbs_.size() - 1u) * 32u;
    for (int bit = 31; bit >= 0; --bit) {
        if ((top & (1u << bit)) != 0) {
            return bits + static_cast<std::size_t>(bit + 1);
        }
    }
    return bits;
}

bool BigUnsigned::test_bit(std::size_t bit_index) const {
    const std::size_t limb_index = bit_index / 32u;
    if (limb_index >= limbs_.size()) {
        return false;
    }
    return (limbs_[limb_index] & (1u << (bit_index % 32u))) != 0;
}

std::string BigUnsigned::to_decimal_string() const {
    if (is_zero()) {
        return "0";
    }

    BigUnsigned copy = *this;
    std::vector<std::uint32_t> chunks;
    while (!copy.is_zero()) {
        chunks.push_back(copy.divide_by_uint32(1000000000u));
    }

    std::ostringstream output;
    output << chunks.back();
    for (std::size_t i = chunks.size() - 1u; i > 0; --i) {
        output << std::setw(9) << std::setfill('0') << chunks[i - 1u];
    }
    return output.str();
}

std::string BigUnsigned::to_hex_string() const {
    if (is_zero()) {
        return "0x0";
    }

    std::ostringstream output;
    output << "0x" << std::hex << std::nouppercase << limbs_.back();
    for (std::size_t i = limbs_.size() - 1u; i > 0; --i) {
        output << std::setw(8) << std::setfill('0') << limbs_[i - 1u];
    }
    return output.str();
}

int BigUnsigned::compare(const BigUnsigned& other) const {
    if (limbs_.size() < other.limbs_.size()) return -1;
    if (limbs_.size() > other.limbs_.size()) return 1;

    for (std::size_t i = limbs_.size(); i > 0; --i) {
        if (limbs_[i - 1u] < other.limbs_[i - 1u]) return -1;
        if (limbs_[i - 1u] > other.limbs_[i - 1u]) return 1;
    }
    return 0;
}

BigUnsigned& BigUnsigned::operator+=(const BigUnsigned& other) {
    const std::size_t max_size = std::max(limbs_.size(), other.limbs_.size());
    limbs_.resize(max_size, 0);

    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < max_size; ++i) {
        const std::uint64_t sum = static_cast<std::uint64_t>(limbs_[i]) +
                                  (i < other.limbs_.size() ? other.limbs_[i] : 0u) +
                                  carry;
        limbs_[i] = static_cast<std::uint32_t>(sum & 0xffffffffu);
        carry = sum >> 32u;
    }
    if (carry != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
    return *this;
}

BigUnsigned& BigUnsigned::operator-=(const BigUnsigned& other) {
    if (*this < other) {
        throw std::underflow_error("BigUnsigned subtraction would become negative");
    }

    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        const std::uint64_t subtrahend = (i < other.limbs_.size() ? other.limbs_[i] : 0u) + borrow;
        if (limbs_[i] < subtrahend) {
            limbs_[i] = static_cast<std::uint32_t>(kLimbBase + limbs_[i] - subtrahend);
            borrow = 1;
        } else {
            limbs_[i] = static_cast<std::uint32_t>(limbs_[i] - subtrahend);
            borrow = 0;
        }
    }
    normalize();
    return *this;
}

BigUnsigned& BigUnsigned::operator*=(const BigUnsigned& other) {
    if (is_zero() || other.is_zero()) {
        limbs_.clear();
        return *this;
    }

    std::vector<std::uint32_t> product(limbs_.size() + other.limbs_.size(), 0);
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < other.limbs_.size(); ++j) {
            const std::uint64_t current = product[i + j] +
                                          static_cast<std::uint64_t>(limbs_[i]) * other.limbs_[j] +
                                          carry;
            product[i + j] = static_cast<std::uint32_t>(current & 0xffffffffu);
            carry = current >> 32u;
        }

        std::size_t index = i + other.limbs_.size();
        while (carry != 0) {
            const std::uint64_t current = static_cast<std::uint64_t>(product[index]) + carry;
            product[index] = static_cast<std::uint32_t>(current & 0xffffffffu);
            carry = current >> 32u;
            ++index;
        }
    }

    limbs_ = std::move(product);
    normalize();
    return *this;
}

BigUnsigned& BigUnsigned::operator/=(const BigUnsigned& other) {
    BigUnsigned quotient;
    BigUnsigned remainder;
    div_mod(*this, other, quotient, remainder);
    *this = quotient;
    return *this;
}

BigUnsigned& BigUnsigned::operator%=(const BigUnsigned& other) {
    BigUnsigned quotient;
    BigUnsigned remainder;
    div_mod(*this, other, quotient, remainder);
    *this = remainder;
    return *this;
}

void BigUnsigned::normalize() {
    while (!limbs_.empty() && limbs_.back() == 0) {
        limbs_.pop_back();
    }
}

void BigUnsigned::shift_left_one() {
    std::uint64_t carry = 0;
    for (std::uint32_t& limb : limbs_) {
        const std::uint64_t shifted = (static_cast<std::uint64_t>(limb) << 1u) | carry;
        limb = static_cast<std::uint32_t>(shifted & 0xffffffffu);
        carry = shifted >> 32u;
    }
    if (carry != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
}

void BigUnsigned::set_bit(std::size_t bit_index) {
    const std::size_t limb_index = bit_index / 32u;
    if (limb_index >= limbs_.size()) {
        limbs_.resize(limb_index + 1u, 0);
    }
    limbs_[limb_index] |= 1u << (bit_index % 32u);
}

void BigUnsigned::multiply_by_uint32(std::uint32_t value) {
    if (is_zero() || value == 1) {
        return;
    }
    if (value == 0) {
        limbs_.clear();
        return;
    }

    std::uint64_t carry = 0;
    for (std::uint32_t& limb : limbs_) {
        const std::uint64_t product = static_cast<std::uint64_t>(limb) * value + carry;
        limb = static_cast<std::uint32_t>(product & 0xffffffffu);
        carry = product >> 32u;
    }
    if (carry != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
}

void BigUnsigned::add_uint32(std::uint32_t value) {
    std::uint64_t carry = value;
    for (std::size_t i = 0; carry != 0 && i < limbs_.size(); ++i) {
        const std::uint64_t sum = static_cast<std::uint64_t>(limbs_[i]) + carry;
        limbs_[i] = static_cast<std::uint32_t>(sum & 0xffffffffu);
        carry = sum >> 32u;
    }
    if (carry != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
}

std::uint32_t BigUnsigned::divide_by_uint32(std::uint32_t value) {
    if (value == 0) {
        throw std::domain_error("division by zero");
    }

    std::uint64_t remainder = 0;
    for (std::size_t i = limbs_.size(); i > 0; --i) {
        const std::uint64_t current = (remainder << 32u) | limbs_[i - 1u];
        limbs_[i - 1u] = static_cast<std::uint32_t>(current / value);
        remainder = current % value;
    }
    normalize();
    return static_cast<std::uint32_t>(remainder);
}

bool operator==(const BigUnsigned& left, const BigUnsigned& right) {
    return left.compare(right) == 0;
}

bool operator!=(const BigUnsigned& left, const BigUnsigned& right) {
    return !(left == right);
}

bool operator<(const BigUnsigned& left, const BigUnsigned& right) {
    return left.compare(right) < 0;
}

bool operator<=(const BigUnsigned& left, const BigUnsigned& right) {
    return left.compare(right) <= 0;
}

bool operator>(const BigUnsigned& left, const BigUnsigned& right) {
    return left.compare(right) > 0;
}

bool operator>=(const BigUnsigned& left, const BigUnsigned& right) {
    return left.compare(right) >= 0;
}

BigUnsigned operator+(BigUnsigned left, const BigUnsigned& right) {
    left += right;
    return left;
}

BigUnsigned operator-(BigUnsigned left, const BigUnsigned& right) {
    left -= right;
    return left;
}

BigUnsigned operator*(BigUnsigned left, const BigUnsigned& right) {
    left *= right;
    return left;
}

BigUnsigned operator/(BigUnsigned left, const BigUnsigned& right) {
    left /= right;
    return left;
}

BigUnsigned operator%(BigUnsigned left, const BigUnsigned& right) {
    left %= right;
    return left;
}

void div_mod(const BigUnsigned& dividend,
             const BigUnsigned& divisor,
             BigUnsigned& quotient,
             BigUnsigned& remainder) {
    if (divisor.is_zero()) {
        throw std::domain_error("division by zero");
    }

    quotient = BigUnsigned();
    remainder = BigUnsigned();

    for (std::size_t bit = dividend.bit_length(); bit > 0; --bit) {
        remainder.shift_left_one();
        if (dividend.test_bit(bit - 1u)) {
            remainder.add_uint32(1);
        }

        if (remainder >= divisor) {
            remainder -= divisor;
            quotient.set_bit(bit - 1u);
        }
    }
    quotient.normalize();
    remainder.normalize();
}

BigUnsigned gcd(BigUnsigned left, BigUnsigned right) {
    while (!right.is_zero()) {
        BigUnsigned remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

BigUnsigned mod_pow(BigUnsigned base, const BigUnsigned& exponent, const BigUnsigned& modulus) {
    if (modulus.is_zero()) {
        throw std::domain_error("mod_pow modulus must be non-zero");
    }
    if (modulus == BigUnsigned(1)) {
        return BigUnsigned(0);
    }

    BigUnsigned result = BigUnsigned(1) % modulus;
    base %= modulus;

    for (std::size_t bit = 0; bit < exponent.bit_length(); ++bit) {
        if (exponent.test_bit(bit)) {
            result = (result * base) % modulus;
        }
        base = (base * base) % modulus;
    }
    return result;
}

} // namespace security::custom_numbers