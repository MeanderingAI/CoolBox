#include "security/fuzzer/fuzzer.h"
#include <cassert>
#include <iostream>

void test_basic_fuzzing() {
    std::cout << "Testing basic fuzzing...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 100;
    config.strategy = security::FuzzStrategy::RANDOM;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int call_count = 0;
    fuzzer.fuzz([&](const std::string& input) {
        call_count++;
        // Do nothing - just count calls
    });
    
    assert(call_count == 100);
    assert(fuzzer.get_results().size() == 100);
    std::cout << "✓ Basic fuzzing test passed\n";
}

void test_crash_detection() {
    std::cout << "Testing crash detection...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::BUFFER_OVERFLOW;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    fuzzer.fuzz([](const std::string& input) {
        if (input.length() > 100) {
            throw std::runtime_error("Buffer overflow!");
        }
    });
    
    assert(fuzzer.get_crash_count() > 0);
    std::cout << "✓ Crash detection test passed (found " << fuzzer.get_crash_count() << " crashes)\n";
}

void test_sql_injection_patterns() {
    std::cout << "Testing SQL injection patterns...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::SQL_INJECTION;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int sql_detected = 0;
    fuzzer.fuzz([&](const std::string& input) {
        if (input.find("DROP TABLE") != std::string::npos ||
            input.find("OR '1'='1") != std::string::npos) {
            sql_detected++;
            throw std::runtime_error("SQL injection detected!");
        }
    });
    
    assert(sql_detected > 0);
    std::cout << "✓ SQL injection patterns test passed (detected " << sql_detected << " attacks)\n";
}

void test_xss_patterns() {
    std::cout << "Testing XSS patterns...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::XSS;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int xss_detected = 0;
    fuzzer.fuzz([&](const std::string& input) {
        if (input.find("<script>") != std::string::npos ||
            input.find("javascript:") != std::string::npos) {
            xss_detected++;
            throw std::runtime_error("XSS detected!");
        }
    });
    
    assert(xss_detected > 0);
    std::cout << "✓ XSS patterns test passed (detected " << xss_detected << " attacks)\n";
}

void test_mutation_strategy() {
    std::cout << "Testing mutation strategy...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 100;
    config.strategy = security::FuzzStrategy::MUTATE;
    config.seed_inputs = {"test", "input", "data"};
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    fuzzer.fuzz([](const std::string& input) {
        // Accept all inputs
    });
    
    assert(fuzzer.get_results().size() == 100);
    std::cout << "✓ Mutation strategy test passed\n";
}

void test_boundary_cases() {
    std::cout << "Testing boundary cases...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::BOUNDARY;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int empty_strings = 0;
    int long_strings = 0;
    
    fuzzer.fuzz([&](const std::string& input) {
        if (input.empty()) empty_strings++;
        if (input.length() > 200) long_strings++;
    });
    
    assert(empty_strings > 0);
    std::cout << "✓ Boundary cases test passed (empty: " << empty_strings 
              << ", long: " << long_strings << ")\n";
}

void test_statistics() {
    std::cout << "Testing statistics...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 100;
    config.strategy = security::FuzzStrategy::ALL;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    fuzzer.fuzz([](const std::string& input) {
        if (input.length() > 500) {
            throw std::runtime_error("Too long!");
        }
    });
    
    auto stats = fuzzer.get_statistics();
    assert(stats["total_iterations"] == 100);
    assert(stats.find("crashes") != stats.end());
    assert(stats.find("exceptions") != stats.end());
    
    std::cout << "✓ Statistics test passed\n";
}

void test_format_string_patterns() {
    std::cout << "Testing format string patterns...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::FORMAT;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int format_detected = 0;
    fuzzer.fuzz([&](const std::string& input) {
        if (input.find("%n") != std::string::npos ||
            input.find("%s%s") != std::string::npos) {
            format_detected++;
            throw std::runtime_error("Format string attack!");
        }
    });
    
    assert(format_detected > 0);
    std::cout << "✓ Format string patterns test passed (detected " << format_detected << " attacks)\n";
}

void test_integer_overflow_patterns() {
    std::cout << "Testing integer overflow patterns...\n";
    
    security::FuzzConfig config;
    config.max_iterations = 50;
    config.strategy = security::FuzzStrategy::INTEGER_OVERFLOW;
    config.verbose = false;
    
    security::Fuzzer fuzzer(config);
    
    int overflows = 0;
    fuzzer.fuzz([&](const std::string& input) {
        try {
            long long val = std::stoll(input);
            if (val > INT32_MAX || val < INT32_MIN) {
                overflows++;
                throw std::runtime_error("Integer overflow!");
            }
        } catch (const std::invalid_argument&) {
            // Invalid input
        } catch (const std::out_of_range&) {
            overflows++;
            throw;
        }
    });
    
    assert(overflows > 0);
    std::cout << "✓ Integer overflow patterns test passed (detected " << overflows << " overflows)\n";
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║    Fuzzer Unit Tests                 ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";

    try {
        test_basic_fuzzing();
        test_crash_detection();
        test_sql_injection_patterns();
        test_xss_patterns();
        test_mutation_strategy();
        test_boundary_cases();
        test_statistics();
        test_format_string_patterns();
        test_integer_overflow_patterns();
        
        std::cout << "\n╔═══════════════════════════════════════╗\n";
        std::cout << "║   All Tests Passed! ✓                ║\n";
        std::cout << "╚═══════════════════════════════════════╝\n\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed: " << e.what() << "\n";
        return 1;
    }
}
