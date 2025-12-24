#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "security/fuzzer/fuzzer.h"

namespace security {

struct Port {
    int number;
    std::string protocol;  // "tcp", "udp", "http"
    std::string service;   // Identified service name
    bool open;
    bool vulnerable;       // Set by fuzzer if vulnerabilities found
    std::vector<std::string> vulnerabilities;
};

struct Host {
    std::string ip;
    std::string hostname;
    bool alive;
    std::vector<Port> ports;
    double response_time_ms;
    std::string os_guess;
};

struct NetworkTopology {
    std::vector<Host> hosts;
    std::map<std::string, std::vector<std::string>> connections;  // ip -> connected_ips
};

struct ScanConfig {
    std::vector<int> ports_to_scan = {21, 22, 23, 25, 53, 80, 110, 143, 443, 445, 3306, 3389, 5432, 8080, 8443, 9000, 9001, 9002};
    bool scan_tcp = true;
    bool scan_udp = false;
    bool scan_http = true;
    bool perform_fuzzing = false;
    int timeout_ms = 1000;
    int max_threads = 10;
    bool verbose = false;
};

class NetworkScanner {
private:
    ScanConfig config_;
    NetworkTopology topology_;
    
    // Host discovery
    bool ping_host(const std::string& ip);
    std::vector<std::string> discover_hosts(const std::string& network_range);
    
    // Port scanning
    bool scan_tcp_port(const std::string& ip, int port);
    bool scan_udp_port(const std::string& ip, int port);
    std::vector<Port> scan_ports(const std::string& ip);
    
    // Service detection
    std::string detect_service(const std::string& ip, int port, const std::string& protocol);
    std::string guess_os(const Host& host);
    
    // Fuzzing
    void fuzz_service(Host& host, Port& port);
    
    // Graph generation
    std::string generate_dot_graph() const;
    void detect_connections();

public:
    NetworkScanner(const ScanConfig& config = ScanConfig());
    
    // Scan operations
    void scan_network(const std::string& network_range);
    void scan_single_host(const std::string& ip);
    
    // Get results
    const NetworkTopology& get_topology() const { return topology_; }
    std::vector<Host> get_alive_hosts() const;
    std::vector<Host> get_vulnerable_hosts() const;
    
    // Output
    void print_results() const;
    void print_topology_ascii() const;
    void export_topology(const std::string& filename, const std::string& format = "dot") const;
    
    // Statistics
    std::map<std::string, int> get_statistics() const;
};

// Helper functions
std::vector<std::string> parse_network_range(const std::string& range);
std::string ip_to_string(uint32_t ip);
uint32_t string_to_ip(const std::string& ip);

} // namespace security
