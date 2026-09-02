#pragma once

#include <string>
#include <vector>
#include <functional>
#include <random>
#include <map>
#include <chrono>

namespace security {

enum class FuzzStrategy {
    RANDOM,           // Completely random bytes
    MUTATE,           // Mutate existing inputs
    BOUNDARY,         // Test boundary conditions
    FORMAT,           // Format string attacks
    SQL_INJECTION,    // SQL injection patterns
    XSS,              // Cross-site scripting patterns
    BUFFER_OVERFLOW,  // Buffer overflow patterns
    INTEGER_OVERFLOW, // Integer overflow patterns
    ALL               // Use all strategies
};

struct FuzzResult {
    std::string input;
    bool crashed;
    bool timeout;
    bool exception_thrown;
    std::string exception_message;
    double execution_time_ms;
    int exit_code;
    std::string output;
};

struct FuzzConfig {
    size_t max_iterations = 1000;
    size_t max_input_length = 1024;
    size_t timeout_ms = 1000;
    FuzzStrategy strategy = FuzzStrategy::ALL;
    std::vector<std::string> seed_inputs;
    bool verbose = false;
    bool stop_on_crash = false;
};

class Fuzzer {
private:
    FuzzConfig config_;
    std::mt19937 rng_;
    std::vector<FuzzResult> results_;
    std::map<FuzzStrategy, std::vector<std::string>> pattern_db_;
    
    // Random generation
    std::string generate_random_bytes(size_t length);
    std::string generate_random_string(size_t length);
    
    // Mutation strategies
    std::string mutate_flip_bits(const std::string& input);
    std::string mutate_insert_bytes(const std::string& input);
    std::string mutate_delete_bytes(const std::string& input);
    std::string mutate_replace_bytes(const std::string& input);
    std::string mutate_shuffle(const std::string& input);
    
    // Attack pattern generation
    std::string generate_boundary_case();
    std::string generate_format_string_attack();
    std::string generate_sql_injection();
    std::string generate_xss_payload();
    std::string generate_buffer_overflow();
    std::string generate_integer_overflow();
    
    // Input generation based on strategy
    std::string generate_input(FuzzStrategy strategy, const std::string& seed = "");
    
    // Initialize attack pattern database
    void init_pattern_db();

public:
    Fuzzer(const FuzzConfig& config = FuzzConfig());
    
    // Fuzz a function that takes string input
    void fuzz(std::function<void(const std::string&)> target);
    
    // Fuzz a function with custom validation
    void fuzz_with_validator(
        std::function<void(const std::string&)> target,
        std::function<bool(const FuzzResult&)> validator
    );
    
    // Get fuzzing results
    const std::vector<FuzzResult>& get_results() const { return results_; }
    
    // Get crash count
    size_t get_crash_count() const;
    
    // Get statistics
    std::map<std::string, size_t> get_statistics() const;
    
    // Print report
    void print_report() const;
    
    // Export results to file
    void export_results(const std::string& filename) const;
};

// Helper class for coverage-guided fuzzing
class CoverageFuzzer {
private:
    Fuzzer fuzzer_;
    std::map<size_t, size_t> coverage_map_;
    
public:
    CoverageFuzzer(const FuzzConfig& config = FuzzConfig());
    
    // Fuzz with coverage feedback
    void fuzz_with_coverage(
        std::function<void(const std::string&)> target,
        std::function<std::vector<size_t>()> get_coverage
    );
    
    const std::map<size_t, size_t>& get_coverage() const { return coverage_map_; }
};

// Network protocol fuzzer
class NetworkFuzzer {
private:
    Fuzzer fuzzer_;
    std::string host_;
    int port_;
    
public:
    NetworkFuzzer(const std::string& host, int port, const FuzzConfig& config = FuzzConfig());
    
    // Fuzz a network service
    void fuzz_tcp();
    void fuzz_udp();
    void fuzz_http();
    
    // Get results
    const std::vector<FuzzResult>& get_results() const { return fuzzer_.get_results(); }
    
    // Print report
    void print_report() const { fuzzer_.print_report(); }
    
    // Get statistics
    std::map<std::string, size_t> get_statistics() const { return fuzzer_.get_statistics(); }
};

} // namespace security
