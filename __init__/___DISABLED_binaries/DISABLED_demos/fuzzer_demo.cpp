#include "security/fuzzer/fuzzer.h"
#include <iostream>
#include <cstring>
#include <vector>

// Example 1: Vulnerable function with buffer overflow
void vulnerable_strcpy(const std::string& input) {
    char buffer[64];
    if (input.length() > 64) {
        throw std::runtime_error("Buffer overflow detected!");
    }
    std::strcpy(buffer, input.c_str());
}

// Example 2: Function vulnerable to SQL injection
void process_sql_input(const std::string& input) {
    // Simulated SQL processing
    if (input.find("DROP TABLE") != std::string::npos ||
        input.find("'; ") != std::string::npos) {
        throw std::runtime_error("SQL injection attempt detected!");
    }
}

// Example 3: Function vulnerable to format string attacks
void process_format_string(const std::string& input) {
    if (input.find("%n") != std::string::npos ||
        input.find("%s%s%s") != std::string::npos) {
        throw std::runtime_error("Format string attack detected!");
    }
}

// Example 4: Integer overflow vulnerability
void process_integer(const std::string& input) {
    try {
        long long value = std::stoll(input);
        if (value > INT32_MAX || value < INT32_MIN) {
            throw std::runtime_error("Integer overflow detected!");
        }
    } catch (const std::invalid_argument&) {
        // Invalid input
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Integer out of range!");
    }
}

// Example 5: XSS vulnerability detector
void process_html_input(const std::string& input) {
    if (input.find("<script>") != std::string::npos ||
        input.find("javascript:") != std::string::npos ||
        input.find("onerror=") != std::string::npos) {
        throw std::runtime_error("XSS attack detected!");
    }
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║        Security Fuzzing Tool Demo               ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    // Test 1: Buffer Overflow Detection
    {
        std::cout << "\n[Test 1] Fuzzing for Buffer Overflows\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 1000;
        config.strategy = security::FuzzStrategy::BUFFER_OVERFLOW;
        config.verbose = true;
        config.stop_on_crash = false;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(vulnerable_strcpy);
        fuzzer.print_report();
    }

    // Test 2: SQL Injection Detection
    {
        std::cout << "\n[Test 2] Fuzzing for SQL Injections\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 500;
        config.strategy = security::FuzzStrategy::SQL_INJECTION;
        config.verbose = true;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(process_sql_input);
        fuzzer.print_report();
    }

    // Test 3: Format String Attacks
    {
        std::cout << "\n[Test 3] Fuzzing for Format String Attacks\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 500;
        config.strategy = security::FuzzStrategy::FORMAT;
        config.verbose = true;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(process_format_string);
        fuzzer.print_report();
    }

    // Test 4: Integer Overflows
    {
        std::cout << "\n[Test 4] Fuzzing for Integer Overflows\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 500;
        config.strategy = security::FuzzStrategy::INTEGER_OVERFLOW;
        config.verbose = true;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(process_integer);
        fuzzer.print_report();
    }

    // Test 5: XSS Attacks
    {
        std::cout << "\n[Test 5] Fuzzing for XSS Attacks\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 500;
        config.strategy = security::FuzzStrategy::XSS;
        config.verbose = true;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(process_html_input);
        fuzzer.print_report();
    }

    // Test 6: Comprehensive fuzzing with all strategies
    {
        std::cout << "\n[Test 6] Comprehensive Fuzzing (All Strategies)\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 2000;
        config.strategy = security::FuzzStrategy::ALL;
        config.verbose = false; // Don't print every exception
        config.seed_inputs = {"test", "admin", "user", "SELECT * FROM users"};
        
        security::Fuzzer fuzzer(config);
        
        // Test all functions
        std::cout << "\nFuzzing vulnerable_strcpy...\n";
        fuzzer.fuzz(vulnerable_strcpy);
        
        std::cout << "\nFuzzing process_sql_input...\n";
        security::Fuzzer fuzzer2(config);
        fuzzer2.fuzz(process_sql_input);
        
        std::cout << "\nFuzzing process_html_input...\n";
        security::Fuzzer fuzzer3(config);
        fuzzer3.fuzz(process_html_input);
        
        // Print combined statistics
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║   Combined Fuzzing Statistics          ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        
        auto stats1 = fuzzer.get_statistics();
        auto stats2 = fuzzer2.get_statistics();
        auto stats3 = fuzzer3.get_statistics();
        
        std::cout << "\nTotal tests run: " 
                  << (stats1["total_iterations"] + stats2["total_iterations"] + stats3["total_iterations"]) << "\n";
        std::cout << "Total crashes: " 
                  << (stats1["crashes"] + stats2["crashes"] + stats3["crashes"]) << "\n";
        std::cout << "Total exceptions: " 
                  << (stats1["exceptions"] + stats2["exceptions"] + stats3["exceptions"]) << "\n";
    }

    // Test 7: Mutation-based fuzzing
    {
        std::cout << "\n[Test 7] Mutation-based Fuzzing\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 1000;
        config.strategy = security::FuzzStrategy::MUTATE;
        config.seed_inputs = {
            "normal input",
            "SELECT * FROM users WHERE id=1",
            "<div>content</div>",
            "123456"
        };
        config.verbose = false;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz(process_sql_input);
        fuzzer.print_report();
        
        // Export results
        fuzzer.export_results("fuzz_results.csv");
        std::cout << "\nResults exported to fuzz_results.csv\n";
    }

    // Test 8: Boundary value testing
    {
        std::cout << "\n[Test 8] Boundary Value Testing\n";
        std::cout << std::string(60, '=') << "\n";
        
        security::FuzzConfig config;
        config.max_iterations = 500;
        config.strategy = security::FuzzStrategy::BOUNDARY;
        config.verbose = true;
        
        security::Fuzzer fuzzer(config);
        fuzzer.fuzz([](const std::string& input) {
            if (input.empty()) {
                throw std::runtime_error("Empty input!");
            }
            if (input.length() > 256) {
                throw std::runtime_error("Input too long!");
            }
            if (input.find('\0') != std::string::npos) {
                throw std::runtime_error("Null byte detected!");
            }
        });
        fuzzer.print_report();
    }

    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║         Fuzzing Demo Complete!                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "Summary:\n";
    std::cout << "- Tested 8 different fuzzing scenarios\n";
    std::cout << "- Detected buffer overflows, SQL injections, XSS, format strings, and integer overflows\n";
    std::cout << "- Demonstrated multiple fuzzing strategies (random, mutation, pattern-based)\n";
    std::cout << "- Exported results to CSV for analysis\n\n";

    return 0;
}
