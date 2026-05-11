#include "../headers/fuzzer.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cstring>

namespace security {

// ===================================================================
// Fuzzer
// ===================================================================

Fuzzer::Fuzzer(const FuzzConfig& config)
    : config_(config)
    , rng_(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    init_pattern_db();
}

void Fuzzer::init_pattern_db() {
    pattern_db_[FuzzStrategy::FORMAT] = {
        "%s%s%s%s%s%s%s%s%s%s",
        "%x%x%x%x%x%x%x%x",
        "%n%n%n%n%n%n%n%n",
        "%d%d%d%d%d%d%d%d",
        "%.1024d",
        "%.16384s",
        "%p%p%p%p%p%p%p%p",
        "AAAA%08x.%08x.%08x.%08x",
    };

    pattern_db_[FuzzStrategy::SQL_INJECTION] = {
        "' OR '1'='1",
        "'; DROP TABLE users;--",
        "\" OR \"\"=\"",
        "1; SELECT * FROM users",
        "1 UNION SELECT null,null,null--",
        "admin'--",
        "' OR 1=1#",
        "'; EXEC xp_cmdshell('whoami');--",
        "1' AND '1'='1",
        "') OR ('1'='1",
    };

    pattern_db_[FuzzStrategy::XSS] = {
        "<script>alert('XSS')</script>",
        "<img src=x onerror=alert(1)>",
        "\"><script>alert(document.cookie)</script>",
        "<svg onload=alert(1)>",
        "javascript:alert(1)",
        "<body onload=alert(1)>",
        "<iframe src='javascript:alert(1)'>",
        "'-alert(1)-'",
        "<div style=\"background:url(javascript:alert(1))\">",
    };

    pattern_db_[FuzzStrategy::BOUNDARY] = {
        "",
        std::string(1, '\0'),
        std::string(1024, 'A'),
        std::string(65536, 'B'),
        "\xff\xfe",
        "\x00\x00\x00\x00",
        std::string(1, '\x7f'),
        std::string(1, '\x80'),
        "-1",
        "0",
        "2147483647",
        "-2147483648",
        "4294967295",
        "9999999999999999999",
    };

    pattern_db_[FuzzStrategy::BUFFER_OVERFLOW] = {
        std::string(256, 'A'),
        std::string(1024, 'A'),
        std::string(4096, 'A'),
        std::string(8192, '\x41'),
        std::string(256, '\x90') + std::string(4, '\xcc'),
    };

    pattern_db_[FuzzStrategy::INTEGER_OVERFLOW] = {
        "2147483647",
        "2147483648",
        "-2147483648",
        "-2147483649",
        "4294967295",
        "4294967296",
        "9223372036854775807",
        "9223372036854775808",
        "18446744073709551615",
        "0",
        "-1",
        "0xFFFFFFFF",
        "0x7FFFFFFF",
        "0x80000000",
    };
}

std::string Fuzzer::generate_random_bytes(size_t length) {
    std::uniform_int_distribution<int> dist(0, 255);
    std::string result(length, '\0');
    for (size_t i = 0; i < length; ++i)
        result[i] = static_cast<char>(dist(rng_));
    return result;
}

std::string Fuzzer::generate_random_string(size_t length) {
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "!@#$%^&*()-_=+[]{}|;:',.<>?/~`";
    std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(chars) - 2));
    std::string result(length, '\0');
    for (size_t i = 0; i < length; ++i)
        result[i] = chars[dist(rng_)];
    return result;
}

std::string Fuzzer::mutate_flip_bits(const std::string& input) {
    if (input.empty()) return generate_random_bytes(8);
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, result.size() - 1);
    std::uniform_int_distribution<int> bit_dist(0, 7);
    size_t pos = pos_dist(rng_);
    result[pos] ^= static_cast<char>(1 << bit_dist(rng_));
    return result;
}

std::string Fuzzer::mutate_insert_bytes(const std::string& input) {
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, result.size());
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<size_t> count_dist(1, 8);
    size_t pos = pos_dist(rng_);
    size_t count = count_dist(rng_);
    for (size_t i = 0; i < count; ++i)
        result.insert(pos, 1, static_cast<char>(byte_dist(rng_)));
    return result;
}

std::string Fuzzer::mutate_delete_bytes(const std::string& input) {
    if (input.size() <= 1) return input;
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, result.size() - 1);
    std::uniform_int_distribution<size_t> count_dist(1, std::min<size_t>(4, result.size()));
    size_t pos = pos_dist(rng_);
    size_t count = std::min(count_dist(rng_), result.size() - pos);
    result.erase(pos, count);
    return result;
}

std::string Fuzzer::mutate_replace_bytes(const std::string& input) {
    if (input.empty()) return generate_random_bytes(8);
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, result.size() - 1);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    size_t pos = pos_dist(rng_);
    result[pos] = static_cast<char>(byte_dist(rng_));
    return result;
}

std::string Fuzzer::mutate_shuffle(const std::string& input) {
    std::string result = input;
    std::shuffle(result.begin(), result.end(), rng_);
    return result;
}

std::string Fuzzer::generate_boundary_case() {
    auto& cases = pattern_db_[FuzzStrategy::BOUNDARY];
    std::uniform_int_distribution<size_t> dist(0, cases.size() - 1);
    return cases[dist(rng_)];
}

std::string Fuzzer::generate_format_string_attack() {
    auto& patterns = pattern_db_[FuzzStrategy::FORMAT];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_sql_injection() {
    auto& patterns = pattern_db_[FuzzStrategy::SQL_INJECTION];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_xss_payload() {
    auto& patterns = pattern_db_[FuzzStrategy::XSS];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_buffer_overflow() {
    auto& patterns = pattern_db_[FuzzStrategy::BUFFER_OVERFLOW];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_integer_overflow() {
    auto& patterns = pattern_db_[FuzzStrategy::INTEGER_OVERFLOW];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_input(FuzzStrategy strategy, const std::string& seed) {
    std::uniform_int_distribution<size_t> len_dist(1, config_.max_input_length);

    switch (strategy) {
        case FuzzStrategy::RANDOM:
            return generate_random_bytes(len_dist(rng_));
        case FuzzStrategy::MUTATE: {
            if (seed.empty()) return generate_random_string(len_dist(rng_));
            std::uniform_int_distribution<int> method(0, 4);
            switch (method(rng_)) {
                case 0: return mutate_flip_bits(seed);
                case 1: return mutate_insert_bytes(seed);
                case 2: return mutate_delete_bytes(seed);
                case 3: return mutate_replace_bytes(seed);
                default: return mutate_shuffle(seed);
            }
        }
        case FuzzStrategy::BOUNDARY:
            return generate_boundary_case();
        case FuzzStrategy::FORMAT:
            return generate_format_string_attack();
        case FuzzStrategy::SQL_INJECTION:
            return generate_sql_injection();
        case FuzzStrategy::XSS:
            return generate_xss_payload();
        case FuzzStrategy::BUFFER_OVERFLOW:
            return generate_buffer_overflow();
        case FuzzStrategy::INTEGER_OVERFLOW:
            return generate_integer_overflow();
        case FuzzStrategy::ALL: {
            std::uniform_int_distribution<int> strat_dist(0, 7);
            auto s = static_cast<FuzzStrategy>(strat_dist(rng_));
            return generate_input(s, seed);
        }
    }
    return generate_random_bytes(len_dist(rng_));
}

void Fuzzer::fuzz(std::function<void(const std::string&)> target) {
    fuzz_with_validator(target, nullptr);
}

void Fuzzer::fuzz_with_validator(
    std::function<void(const std::string&)> target,
    std::function<bool(const FuzzResult&)> validator)
{
    results_.clear();
    std::string seed;
    if (!config_.seed_inputs.empty()) {
        std::uniform_int_distribution<size_t> seed_dist(0, config_.seed_inputs.size() - 1);
        seed = config_.seed_inputs[seed_dist(rng_)];
    }

    for (size_t i = 0; i < config_.max_iterations; ++i) {
        std::string input = generate_input(config_.strategy, seed);
        FuzzResult result;
        result.input = input;
        result.crashed = false;
        result.timeout = false;
        result.exception_thrown = false;
        result.exit_code = 0;

        auto start = std::chrono::steady_clock::now();
        try {
            target(input);
        } catch (const std::exception& e) {
            result.exception_thrown = true;
            result.exception_message = e.what();
        } catch (...) {
            result.crashed = true;
            result.exception_message = "Unknown exception";
        }
        auto end = std::chrono::steady_clock::now();
        result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (result.execution_time_ms > config_.timeout_ms)
            result.timeout = true;

        if (validator && !validator(result))
            result.crashed = true;

        results_.push_back(result);

        if (config_.verbose && (result.crashed || result.exception_thrown)) {
            std::cerr << "[FUZZ] Iteration " << i
                      << ": " << (result.crashed ? "CRASH" : "EXCEPTION")
                      << " - " << result.exception_message << "\n";
        }

        if (config_.stop_on_crash && result.crashed)
            break;

        // Use interesting inputs as seeds for mutation
        if (result.exception_thrown || result.timeout)
            seed = input;
    }
}

size_t Fuzzer::get_crash_count() const {
    return static_cast<size_t>(std::count_if(results_.begin(), results_.end(),
        [](const FuzzResult& r) { return r.crashed; }));
}

std::map<std::string, size_t> Fuzzer::get_statistics() const {
    std::map<std::string, size_t> stats;
    stats["total_iterations"] = results_.size();
    stats["crashes"] = 0;
    stats["exceptions"] = 0;
    stats["timeouts"] = 0;

    for (auto& r : results_) {
        if (r.crashed) ++stats["crashes"];
        if (r.exception_thrown) ++stats["exceptions"];
        if (r.timeout) ++stats["timeouts"];
    }
    return stats;
}

void Fuzzer::print_report() const {
    auto stats = get_statistics();
    std::cout << "\n=== Fuzzing Report ===\n"
              << "Total iterations: " << stats["total_iterations"] << "\n"
              << "Crashes:          " << stats["crashes"] << "\n"
              << "Exceptions:       " << stats["exceptions"] << "\n"
              << "Timeouts:         " << stats["timeouts"] << "\n";

    if (!results_.empty()) {
        double total_ms = 0;
        for (auto& r : results_) total_ms += r.execution_time_ms;
        std::cout << "Avg exec time:    " << std::fixed << std::setprecision(3)
                  << (total_ms / results_.size()) << " ms\n";
    }
    std::cout << "======================\n\n";
}

void Fuzzer::export_results(const std::string& filename) const {
    std::ofstream f(filename);
    if (!f) return;

    f << "iteration,crashed,exception,timeout,exec_time_ms,message\n";
    for (size_t i = 0; i < results_.size(); ++i) {
        auto& r = results_[i];
        f << i << ","
          << (r.crashed ? 1 : 0) << ","
          << (r.exception_thrown ? 1 : 0) << ","
          << (r.timeout ? 1 : 0) << ","
          << std::fixed << std::setprecision(3) << r.execution_time_ms << ","
          << "\"" << r.exception_message << "\"\n";
    }
}

// ===================================================================
// CoverageFuzzer
// ===================================================================

CoverageFuzzer::CoverageFuzzer(const FuzzConfig& config)
    : fuzzer_(config) {}

void CoverageFuzzer::fuzz_with_coverage(
    std::function<void(const std::string&)> target,
    std::function<std::vector<size_t>()> get_coverage)
{
    fuzzer_.fuzz([&](const std::string& input) {
        target(input);
        auto cov = get_coverage();
        for (size_t addr : cov)
            ++coverage_map_[addr];
    });
}

// ===================================================================
// NetworkFuzzer (stub – networking requires platform-specific code)
// ===================================================================

NetworkFuzzer::NetworkFuzzer(const std::string& host, int port, const FuzzConfig& config)
    : fuzzer_(config), host_(host), port_(port) {}

void NetworkFuzzer::fuzz_tcp() {
    // Placeholder: real implementation would open TCP sockets
    fuzzer_.fuzz([](const std::string&) {});
}

void NetworkFuzzer::fuzz_udp() {
    fuzzer_.fuzz([](const std::string&) {});
}

void NetworkFuzzer::fuzz_http() {
    fuzzer_.fuzz([](const std::string&) {});
}

} // namespace security
