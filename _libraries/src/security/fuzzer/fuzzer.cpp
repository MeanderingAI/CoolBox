#include "security/fuzzer/fuzzer.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace security {

Fuzzer::Fuzzer(const FuzzConfig& config) : config_(config) {
    std::random_device rd;
    rng_ = std::mt19937(rd());
    init_pattern_db();
}

void Fuzzer::init_pattern_db() {
    // SQL Injection patterns
    pattern_db_[FuzzStrategy::SQL_INJECTION] = {
        "' OR '1'='1",
        "'; DROP TABLE users--",
        "1' UNION SELECT NULL--",
        "admin'--",
        "' OR 1=1--",
        "1'; WAITFOR DELAY '00:00:05'--",
        "' AND 1=0 UNION ALL SELECT 'admin', 'password'--",
        "' HAVING 1=1--",
        "'; EXEC sp_MSForEachTable 'DROP TABLE ?'--"
    };
    
    // XSS patterns
    pattern_db_[FuzzStrategy::XSS] = {
        "<script>alert('XSS')</script>",
        "<img src=x onerror=alert('XSS')>",
        "<svg onload=alert('XSS')>",
        "javascript:alert('XSS')",
        "<iframe src=javascript:alert('XSS')>",
        "<body onload=alert('XSS')>",
        "<input onfocus=alert('XSS') autofocus>",
        "\"><script>alert(String.fromCharCode(88,83,83))</script>",
        "<scr<script>ipt>alert('XSS')</scr</script>ipt>"
    };
    
    // Format string patterns
    pattern_db_[FuzzStrategy::FORMAT] = {
        "%s%s%s%s%s%s%s%s%s%s",
        "%x%x%x%x%x%x%x%x",
        "%n%n%n%n%n",
        "%s%p%x%d",
        "%.1000d%.1000d%.1000d",
        "%08x.%08x.%08x.%08x",
        "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
    };
    
    // Buffer overflow patterns
    pattern_db_[FuzzStrategy::BUFFER_OVERFLOW] = {
        std::string(256, 'A'),
        std::string(512, 'A'),
        std::string(1024, 'A'),
        std::string(4096, 'A'),
        std::string(256, '\x41') + std::string(8, '\x90') + "\xcc\xcc\xcc\xcc",
    };
    
    // Integer overflow patterns
    pattern_db_[FuzzStrategy::INTEGER_OVERFLOW] = {
        "2147483647",  // INT_MAX
        "2147483648",  // INT_MAX + 1
        "-2147483648", // INT_MIN
        "-2147483649", // INT_MIN - 1
        "4294967295",  // UINT_MAX
        "4294967296",  // UINT_MAX + 1
        "-1",
        "0"
    };
    
    // Boundary cases
    pattern_db_[FuzzStrategy::BOUNDARY] = {
        "",
        " ",
        "\n",
        "\r\n",
        "\t",
        "\x00",
        "\xff",
        std::string(1, '\0'),
        std::string(255, 'A'),
        std::string(256, 'A'),
        "\\",
        "/",
        "..",
        "../",
        "~",
        "`"
    };
}

std::string Fuzzer::generate_random_bytes(size_t length) {
    std::uniform_int_distribution<int> dist(0, 255);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += static_cast<char>(dist(rng_));
    }
    return result;
}

std::string Fuzzer::generate_random_string(size_t length) {
    static const char charset[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "!@#$%^&*()_+-=[]{}|;:,.<>?/";
    
    std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(rng_)];
    }
    return result;
}

std::string Fuzzer::mutate_flip_bits(const std::string& input) {
    if (input.empty()) return input;
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, input.length() - 1);
    std::uniform_int_distribution<int> bit_dist(0, 7);
    
    size_t pos = pos_dist(rng_);
    int bit = bit_dist(rng_);
    result[pos] ^= (1 << bit);
    return result;
}

std::string Fuzzer::mutate_insert_bytes(const std::string& input) {
    std::uniform_int_distribution<size_t> pos_dist(0, input.length());
    std::uniform_int_distribution<size_t> len_dist(1, 16);
    
    size_t pos = pos_dist(rng_);
    size_t len = len_dist(rng_);
    std::string insert = generate_random_bytes(len);
    
    return input.substr(0, pos) + insert + input.substr(pos);
}

std::string Fuzzer::mutate_delete_bytes(const std::string& input) {
    if (input.empty()) return input;
    std::uniform_int_distribution<size_t> pos_dist(0, input.length() - 1);
    std::uniform_int_distribution<size_t> len_dist(1, std::min<size_t>(16, input.length()));
    
    size_t pos = pos_dist(rng_);
    size_t len = std::min(len_dist(rng_), input.length() - pos);
    
    return input.substr(0, pos) + input.substr(pos + len);
}

std::string Fuzzer::mutate_replace_bytes(const std::string& input) {
    if (input.empty()) return input;
    std::string result = input;
    std::uniform_int_distribution<size_t> pos_dist(0, input.length() - 1);
    
    size_t pos = pos_dist(rng_);
    result[pos] = static_cast<char>(std::uniform_int_distribution<int>(0, 255)(rng_));
    return result;
}

std::string Fuzzer::mutate_shuffle(const std::string& input) {
    std::string result = input;
    std::shuffle(result.begin(), result.end(), rng_);
    return result;
}

std::string Fuzzer::generate_boundary_case() {
    const auto& patterns = pattern_db_[FuzzStrategy::BOUNDARY];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_format_string_attack() {
    const auto& patterns = pattern_db_[FuzzStrategy::FORMAT];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_sql_injection() {
    const auto& patterns = pattern_db_[FuzzStrategy::SQL_INJECTION];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_xss_payload() {
    const auto& patterns = pattern_db_[FuzzStrategy::XSS];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_buffer_overflow() {
    const auto& patterns = pattern_db_[FuzzStrategy::BUFFER_OVERFLOW];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_integer_overflow() {
    const auto& patterns = pattern_db_[FuzzStrategy::INTEGER_OVERFLOW];
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

std::string Fuzzer::generate_input(FuzzStrategy strategy, const std::string& seed) {
    if (strategy == FuzzStrategy::RANDOM) {
        std::uniform_int_distribution<size_t> len_dist(1, config_.max_input_length);
        return generate_random_bytes(len_dist(rng_));
    } else if (strategy == FuzzStrategy::MUTATE && !seed.empty()) {
        std::uniform_int_distribution<int> mut_dist(0, 4);
        int mutation = mut_dist(rng_);
        switch (mutation) {
            case 0: return mutate_flip_bits(seed);
            case 1: return mutate_insert_bytes(seed);
            case 2: return mutate_delete_bytes(seed);
            case 3: return mutate_replace_bytes(seed);
            case 4: return mutate_shuffle(seed);
        }
    } else if (strategy == FuzzStrategy::BOUNDARY) {
        return generate_boundary_case();
    } else if (strategy == FuzzStrategy::FORMAT) {
        return generate_format_string_attack();
    } else if (strategy == FuzzStrategy::SQL_INJECTION) {
        return generate_sql_injection();
    } else if (strategy == FuzzStrategy::XSS) {
        return generate_xss_payload();
    } else if (strategy == FuzzStrategy::BUFFER_OVERFLOW) {
        return generate_buffer_overflow();
    } else if (strategy == FuzzStrategy::INTEGER_OVERFLOW) {
        return generate_integer_overflow();
    }
    
    return generate_random_string(std::uniform_int_distribution<size_t>(1, config_.max_input_length)(rng_));
}

void Fuzzer::fuzz(std::function<void(const std::string&)> target) {
    std::cout << "Starting fuzzer with " << config_.max_iterations << " iterations...\n";
    
    for (size_t i = 0; i < config_.max_iterations; ++i) {
        FuzzResult result;
        
        // Determine strategy
        FuzzStrategy strategy = config_.strategy;
        if (strategy == FuzzStrategy::ALL) {
            std::uniform_int_distribution<int> strat_dist(0, 7);
            strategy = static_cast<FuzzStrategy>(strat_dist(rng_));
        }
        
        // Get seed if available
        std::string seed;
        if (!config_.seed_inputs.empty()) {
            std::uniform_int_distribution<size_t> seed_dist(0, config_.seed_inputs.size() - 1);
            seed = config_.seed_inputs[seed_dist(rng_)];
        }
        
        // Generate input
        result.input = generate_input(strategy, seed);
        
        // Execute target
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            target(result.input);
            result.crashed = false;
            result.exception_thrown = false;
        } catch (const std::exception& e) {
            result.crashed = false;
            result.exception_thrown = true;
            result.exception_message = e.what();
        } catch (...) {
            result.crashed = true;
            result.exception_thrown = true;
            result.exception_message = "Unknown exception";
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (result.execution_time_ms > config_.timeout_ms) {
            result.timeout = true;
        }
        
        results_.push_back(result);
        
        if (config_.verbose && (result.crashed || result.exception_thrown)) {
            std::cout << "Iteration " << i << ": ";
            if (result.crashed) std::cout << "CRASH ";
            if (result.exception_thrown) std::cout << "EXCEPTION: " << result.exception_message;
            std::cout << "\n";
        }
        
        if (config_.stop_on_crash && result.crashed) {
            std::cout << "Stopping due to crash\n";
            break;
        }
    }
    
    std::cout << "Fuzzing complete. Total iterations: " << results_.size() << "\n";
}

void Fuzzer::fuzz_with_validator(
    std::function<void(const std::string&)> target,
    std::function<bool(const FuzzResult&)> validator
) {
    // Similar to fuzz() but with custom validation
    fuzz(target);
    
    // Apply validator to all results
    for (auto& result : results_) {
        if (!validator(result)) {
            result.crashed = true;
        }
    }
}

size_t Fuzzer::get_crash_count() const {
    return std::count_if(results_.begin(), results_.end(),
        [](const FuzzResult& r) { return r.crashed || r.exception_thrown; });
}

std::map<std::string, size_t> Fuzzer::get_statistics() const {
    std::map<std::string, size_t> stats;
    stats["total_iterations"] = results_.size();
    stats["crashes"] = std::count_if(results_.begin(), results_.end(),
        [](const FuzzResult& r) { return r.crashed; });
    stats["exceptions"] = std::count_if(results_.begin(), results_.end(),
        [](const FuzzResult& r) { return r.exception_thrown; });
    stats["timeouts"] = std::count_if(results_.begin(), results_.end(),
        [](const FuzzResult& r) { return r.timeout; });
    return stats;
}

void Fuzzer::print_report() const {
    auto stats = get_statistics();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║      Fuzzing Report                   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::cout << "Total Iterations:  " << stats["total_iterations"] << "\n";
    std::cout << "Crashes:           " << stats["crashes"] << "\n";
    std::cout << "Exceptions:        " << stats["exceptions"] << "\n";
    std::cout << "Timeouts:          " << stats["timeouts"] << "\n\n";
    
    if (stats["crashes"] > 0 || stats["exceptions"] > 0) {
        std::cout << "Crash/Exception Details:\n";
        std::cout << std::string(60, '-') << "\n";
        for (size_t i = 0; i < results_.size(); ++i) {
            if (results_[i].crashed || results_[i].exception_thrown) {
                std::cout << "Iteration " << i << ":\n";
                std::cout << "  Input length: " << results_[i].input.length() << " bytes\n";
                if (results_[i].exception_thrown) {
                    std::cout << "  Exception: " << results_[i].exception_message << "\n";
                }
                std::cout << "  Input preview: ";
                std::string preview = results_[i].input.substr(0, std::min<size_t>(50, results_[i].input.length()));
                for (char c : preview) {
                    if (std::isprint(c)) std::cout << c;
                    else std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << std::dec;
                }
                std::cout << "\n\n";
            }
        }
    }
}

void Fuzzer::export_results(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    file << "iteration,crashed,exception,timeout,exec_time_ms,input_length,exception_msg\n";
    for (size_t i = 0; i < results_.size(); ++i) {
        const auto& r = results_[i];
        file << i << ","
             << (r.crashed ? "1" : "0") << ","
             << (r.exception_thrown ? "1" : "0") << ","
             << (r.timeout ? "1" : "0") << ","
             << r.execution_time_ms << ","
             << r.input.length() << ","
             << "\"" << r.exception_message << "\"\n";
    }
}

// Coverage Fuzzer implementation
CoverageFuzzer::CoverageFuzzer(const FuzzConfig& config) : fuzzer_(config) {}

void CoverageFuzzer::fuzz_with_coverage(
    std::function<void(const std::string&)> target,
    std::function<std::vector<size_t>()> get_coverage
) {
    // This is a placeholder for coverage-guided fuzzing
    // In a real implementation, you'd track code coverage and prioritize inputs that increase it
    fuzzer_.fuzz(target);
}

// Network Fuzzer implementation
NetworkFuzzer::NetworkFuzzer(const std::string& host, int port, const FuzzConfig& config)
    : fuzzer_(config), host_(host), port_(port) {}

void NetworkFuzzer::fuzz_tcp() {
    std::cout << "TCP Fuzzing " << host_ << ":" << port_ << "\n";
    
    fuzzer_.fuzz([this](const std::string& input) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid address");
        }
        
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Connection failed");
        }
        
        // Send fuzzed data
        ssize_t sent = send(sockfd, input.c_str(), input.length(), 0);
        if (sent < 0) {
            close(sockfd);
            throw std::runtime_error("Send failed");
        }
        
        // Try to receive response
        char buffer[4096];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        
        close(sockfd);
    });
}

void NetworkFuzzer::fuzz_udp() {
    std::cout << "UDP Fuzzing " << host_ << ":" << port_ << "\n";
    
    fuzzer_.fuzz([this](const std::string& input) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid address");
        }
        
        // Send fuzzed data
        ssize_t sent = sendto(sockfd, input.c_str(), input.length(), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent < 0) {
            close(sockfd);
            throw std::runtime_error("Send failed");
        }
        
        // Try to receive response
        char buffer[4096];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&server_addr, &addr_len);
        
        close(sockfd);
    });
}

void NetworkFuzzer::fuzz_http() {
    std::cout << "HTTP Fuzzing " << host_ << ":" << port_ << "\n";
    
    // HTTP-specific fuzzing patterns
    std::vector<std::string> http_methods = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"};
    std::vector<std::string> http_paths = {"/", "/admin", "/api", "/../../../etc/passwd", "/index.html"};
    
    fuzzer_.fuzz([this, &http_methods, &http_paths](const std::string& input) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid address");
        }
        
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Connection failed");
        }
        
        // Build HTTP request with fuzzed data
        std::string method = http_methods[rand() % http_methods.size()];
        std::string path = http_paths[rand() % http_paths.size()];
        
        std::stringstream request;
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: " << host_ << "\r\n";
        request << "User-Agent: Fuzzer/1.0\r\n";
        request << "Content-Length: " << input.length() << "\r\n";
        request << "X-Fuzzed-Header: " << input.substr(0, std::min<size_t>(50, input.length())) << "\r\n";
        request << "\r\n";
        request << input;
        
        std::string req_str = request.str();
        ssize_t sent = send(sockfd, req_str.c_str(), req_str.length(), 0);
        if (sent < 0) {
            close(sockfd);
            throw std::runtime_error("Send failed");
        }
        
        // Try to receive response
        char buffer[8192];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        
        close(sockfd);
    });
}

} // namespace security
