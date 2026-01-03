#include "security/network_scanner/network_scanner.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstring>

namespace security {

// Helper functions
std::vector<std::string> parse_network_range(const std::string& range) {
    std::vector<std::string> ips;
    
    // Parse CIDR notation (e.g., 192.168.1.0/24)
    size_t slash_pos = range.find('/');
    if (slash_pos != std::string::npos) {
        std::string base_ip = range.substr(0, slash_pos);
        int prefix = std::stoi(range.substr(slash_pos + 1));
        
        uint32_t ip = string_to_ip(base_ip);
        uint32_t mask = (0xFFFFFFFF << (32 - prefix));
        uint32_t network = ip & mask;
        uint32_t broadcast = network | ~mask;
        
        for (uint32_t addr = network + 1; addr < broadcast; ++addr) {
            ips.push_back(ip_to_string(addr));
        }
    } else if (range.find('-') != std::string::npos) {
        // Parse range notation (e.g., 192.168.1.1-254)
        size_t dash_pos = range.find('-');
        std::string base = range.substr(0, range.rfind('.') + 1);
        int start = std::stoi(range.substr(range.rfind('.') + 1, dash_pos - range.rfind('.') - 1));
        int end = std::stoi(range.substr(dash_pos + 1));
        
        for (int i = start; i <= end; ++i) {
            ips.push_back(base + std::to_string(i));
        }
    } else {
        // Single IP
        ips.push_back(range);
    }
    
    return ips;
}

std::string ip_to_string(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    return inet_ntoa(addr);
}

uint32_t string_to_ip(const std::string& ip) {
    struct in_addr addr;
    inet_aton(ip.c_str(), &addr);
    return ntohl(addr.s_addr);
}

// NetworkScanner implementation
NetworkScanner::NetworkScanner(const ScanConfig& config) : config_(config) {}

bool NetworkScanner::ping_host(const std::string& ip) {
    if (config_.verbose) {
        std::cout << "  Checking " << ip << "... ";
        std::cout.flush();
    }
    
    // Try multiple common ports for better detection
    std::vector<int> common_ports = {80, 443, 22, 8080, 9000, 9001, 9002, 3000, 8000};
    
    for (int port : common_ports) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        
        // Set non-blocking
        fcntl(sock, F_SETFL, O_NONBLOCK);
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        
        connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = config_.timeout_ms * 1000;
        
        bool alive = false;
        if (select(sock + 1, nullptr, &fdset, nullptr, &tv) > 0) {
            int error;
            socklen_t len = sizeof(error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
            alive = (error == 0);
        }
        
        close(sock);
        
        if (alive) {
            if (config_.verbose) {
                std::cout << "UP (port " << port << " open)\n";
            }
            return true;
        }
    }
    
    if (config_.verbose) {
        std::cout << "DOWN\n";
    }
    return false;
}

std::vector<std::string> NetworkScanner::discover_hosts(const std::string& network_range) {
    std::vector<std::string> ips = parse_network_range(network_range);
    std::vector<std::string> alive_hosts;
    
    std::cout << "Discovering hosts in " << network_range << " (" << ips.size() << " addresses)...\n";
    
    for (const auto& ip : ips) {
        if (ping_host(ip)) {
            alive_hosts.push_back(ip);
            std::cout << "  [+] " << ip << " is ALIVE\n";
        }
    }
    
    std::cout << "\n";
    return alive_hosts;
}

bool NetworkScanner::scan_tcp_port(const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = config_.timeout_ms * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    bool open = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);
    
    return open;
}

bool NetworkScanner::scan_udp_port(const std::string& ip, int port) {
    // UDP port scanning is less reliable
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = config_.timeout_ms * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    char data[] = "\x00";
    sendto(sock, data, sizeof(data), 0, (struct sockaddr*)&addr, sizeof(addr));
    
    char buffer[1024];
    bool open = (recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr) > 0);
    
    close(sock);
    return open;
}

std::string NetworkScanner::detect_service(const std::string& ip, int port, const std::string& protocol) {
    // Common port -> service mapping
    std::map<int, std::string> common_services = {
        {21, "ftp"}, {22, "ssh"}, {23, "telnet"}, {25, "smtp"},
        {53, "dns"}, {80, "http"}, {110, "pop3"}, {143, "imap"},
        {443, "https"}, {445, "smb"}, {3306, "mysql"}, {3389, "rdp"},
        {5432, "postgresql"}, {8080, "http-proxy"}, {8443, "https-alt"},
        {9000, "http"}, {9001, "http"}, {9002, "http"}
    };
    
    if (common_services.find(port) != common_services.end()) {
        return common_services[port];
    }
    
    // Try to detect by banner grabbing
    if (protocol == "tcp") {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "unknown";
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            char buffer[1024];
            ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::string banner(buffer);
                
                if (banner.find("SSH") != std::string::npos) return "ssh";
                if (banner.find("HTTP") != std::string::npos) return "http";
                if (banner.find("FTP") != std::string::npos) return "ftp";
                if (banner.find("SMTP") != std::string::npos) return "smtp";
            }
        }
        close(sock);
    }
    
    return "unknown";
}

std::vector<Port> NetworkScanner::scan_ports(const std::string& ip) {
    std::vector<Port> ports;
    
    for (int port_num : config_.ports_to_scan) {
        Port port;
        port.number = port_num;
        port.open = false;
        port.vulnerable = false;
        
        if (config_.scan_tcp && scan_tcp_port(ip, port_num)) {
            port.protocol = "tcp";
            port.open = true;
            port.service = detect_service(ip, port_num, "tcp");
            ports.push_back(port);
            
            if (config_.verbose) {
                std::cout << "    [+] " << port_num << "/tcp open (" << port.service << ")\n";
            }
        }
        
        if (config_.scan_udp && scan_udp_port(ip, port_num)) {
            port.protocol = "udp";
            port.open = true;
            port.service = detect_service(ip, port_num, "udp");
            ports.push_back(port);
            
            if (config_.verbose) {
                std::cout << "    [+] " << port_num << "/udp open (" << port.service << ")\n";
            }
        }
    }
    
    return ports;
}

std::string NetworkScanner::guess_os(const Host& host) {
    // Simple OS fingerprinting based on open ports
    std::set<int> open_ports;
    for (const auto& port : host.ports) {
        if (port.open) open_ports.insert(port.number);
    }
    
    if (open_ports.count(3389) || open_ports.count(445)) return "Windows";
    if (open_ports.count(22) && open_ports.count(80)) return "Linux";
    if (open_ports.count(22) && open_ports.count(548)) return "macOS";
    
    return "Unknown";
}

void NetworkScanner::fuzz_service(Host& host, Port& port) {
    if (!port.open) return;
    
    FuzzConfig fuzz_config;
    fuzz_config.max_iterations = 50;
    fuzz_config.verbose = false;
    fuzz_config.strategy = FuzzStrategy::ALL;
    
    try {
        if (port.service == "http" || port.service == "https" || port.service == "http-proxy") {
            NetworkFuzzer fuzzer(host.ip, port.number, fuzz_config);
            fuzzer.fuzz_http();
            
            auto stats = fuzzer.get_statistics();
            if (stats["exceptions"] > 0 || stats["crashes"] > 0) {
                port.vulnerable = true;
                port.vulnerabilities.push_back("HTTP fuzzing detected " + 
                    std::to_string(stats["exceptions"]) + " exceptions");
            }
        } else if (port.protocol == "tcp") {
            NetworkFuzzer fuzzer(host.ip, port.number, fuzz_config);
            fuzzer.fuzz_tcp();
            
            auto stats = fuzzer.get_statistics();
            if (stats["exceptions"] > 0 || stats["crashes"] > 0) {
                port.vulnerable = true;
                port.vulnerabilities.push_back("TCP fuzzing detected " + 
                    std::to_string(stats["exceptions"]) + " exceptions");
            }
        }
    } catch (const std::exception& e) {
        // Fuzzing failed - service might be vulnerable
        port.vulnerable = true;
        port.vulnerabilities.push_back("Fuzzing caused error: " + std::string(e.what()));
    }
}

void NetworkScanner::scan_network(const std::string& network_range) {
    std::cout << "Starting network scan of " << network_range << "...\n";
    
    auto ips = discover_hosts(network_range);
    
    std::cout << "Found " << ips.size() << " alive hosts. Scanning ports...\n";
    
    for (const auto& ip : ips) {
        Host host;
        host.ip = ip;
        host.alive = true;
        
        // Try to resolve hostname
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
        
        char hostname[NI_MAXHOST];
        if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, sizeof(hostname), nullptr, 0, 0) == 0) {
            host.hostname = hostname;
        } else {
            host.hostname = ip;
        }
        
        if (config_.verbose) {
            std::cout << "\n  Scanning " << ip << " (" << host.hostname << ")...\n";
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        host.ports = scan_ports(ip);
        auto end = std::chrono::high_resolution_clock::now();
        host.response_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        host.os_guess = guess_os(host);
        
        // Perform fuzzing if enabled
        if (config_.perform_fuzzing) {
            if (config_.verbose) {
                std::cout << "    Fuzzing services...\n";
            }
            for (auto& port : host.ports) {
                fuzz_service(host, port);
            }
        }
        
        topology_.hosts.push_back(host);
    }
    
    detect_connections();
    
    std::cout << "\nScan complete!\n";
}

void NetworkScanner::scan_single_host(const std::string& ip) {
    scan_network(ip);
}

void NetworkScanner::detect_connections() {
    // Simple connection detection based on common network topology
    // In reality, this would require ARP table analysis, traceroute, etc.
    for (size_t i = 0; i < topology_.hosts.size(); ++i) {
        for (size_t j = i + 1; j < topology_.hosts.size(); ++j) {
            // Assume hosts in same /24 subnet are connected
            std::string ip1 = topology_.hosts[i].ip.substr(0, topology_.hosts[i].ip.rfind('.'));
            std::string ip2 = topology_.hosts[j].ip.substr(0, topology_.hosts[j].ip.rfind('.'));
            
            if (ip1 == ip2) {
                topology_.connections[topology_.hosts[i].ip].push_back(topology_.hosts[j].ip);
                topology_.connections[topology_.hosts[j].ip].push_back(topology_.hosts[i].ip);
            }
        }
    }
}

std::vector<Host> NetworkScanner::get_alive_hosts() const {
    std::vector<Host> alive;
    for (const auto& host : topology_.hosts) {
        if (host.alive) alive.push_back(host);
    }
    return alive;
}

std::vector<Host> NetworkScanner::get_vulnerable_hosts() const {
    std::vector<Host> vulnerable;
    for (const auto& host : topology_.hosts) {
        for (const auto& port : host.ports) {
            if (port.vulnerable) {
                vulnerable.push_back(host);
                break;
            }
        }
    }
    return vulnerable;
}

void NetworkScanner::print_results() const {
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         Network Scan Results                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    for (const auto& host : topology_.hosts) {
        std::cout << "Host: " << host.ip;
        if (host.hostname != host.ip) {
            std::cout << " (" << host.hostname << ")";
        }
        std::cout << "\n";
        std::cout << "  Status: " << (host.alive ? "UP" : "DOWN") << "\n";
        std::cout << "  OS Guess: " << host.os_guess << "\n";
        std::cout << "  Response Time: " << host.response_time_ms << " ms\n";
        std::cout << "  Open Ports: " << host.ports.size() << "\n";
        
        for (const auto& port : host.ports) {
            std::cout << "    " << port.number << "/" << port.protocol 
                      << " " << port.service;
            if (port.vulnerable) {
                std::cout << " [VULNERABLE]";
                for (const auto& vuln : port.vulnerabilities) {
                    std::cout << "\n      - " << vuln;
                }
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

void NetworkScanner::print_topology_ascii() const {
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         Network Topology                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    for (const auto& host : topology_.hosts) {
        std::cout << "[" << host.ip << "]";
        if (host.ports.size() > 0) {
            std::cout << " - Ports: ";
            for (size_t i = 0; i < std::min<size_t>(5, host.ports.size()); ++i) {
                std::cout << host.ports[i].number;
                if (i < std::min<size_t>(5, host.ports.size()) - 1) std::cout << ", ";
            }
            if (host.ports.size() > 5) std::cout << "...";
        }
        std::cout << "\n";
        
        auto it = topology_.connections.find(host.ip);
        if (it != topology_.connections.end() && !it->second.empty()) {
            for (const auto& conn : it->second) {
                std::cout << "  └─> " << conn << "\n";
            }
        }
    }
}

std::string NetworkScanner::generate_dot_graph() const {
    std::stringstream dot;
    
    dot << "digraph NetworkTopology {\n";
    dot << "  rankdir=TB;\n";
    dot << "  node [shape=box, style=filled, fillcolor=lightblue];\n\n";
    
    // Add nodes
    for (const auto& host : topology_.hosts) {
        std::string label = host.ip;
        if (host.hostname != host.ip) {
            label += "\\n" + host.hostname;
        }
        label += "\\n" + host.os_guess;
        
        std::string color = "lightblue";
        for (const auto& port : host.ports) {
            if (port.vulnerable) {
                color = "red";
                break;
            }
        }
        
        dot << "  \"" << host.ip << "\" [label=\"" << label << "\", fillcolor=" << color << "];\n";
    }
    
    dot << "\n";
    
    // Add edges
    std::set<std::pair<std::string, std::string>> added_edges;
    for (const auto& [ip, connections] : topology_.connections) {
        for (const auto& conn : connections) {
            auto edge = std::make_pair(std::min(ip, conn), std::max(ip, conn));
            if (added_edges.find(edge) == added_edges.end()) {
                dot << "  \"" << ip << "\" -> \"" << conn << "\" [dir=none];\n";
                added_edges.insert(edge);
            }
        }
    }
    
    dot << "}\n";
    
    return dot.str();
}

void NetworkScanner::export_topology(const std::string& filename, const std::string& format) const {
    if (format == "dot") {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        file << generate_dot_graph();
        file.close();
        std::cout << "Topology exported to " << filename << "\n";
        std::cout << "Generate image with: dot -Tpng " << filename << " -o network_topology.png\n";
    }
}

std::map<std::string, int> NetworkScanner::get_statistics() const {
    std::map<std::string, int> stats;
    stats["total_hosts"] = topology_.hosts.size();
    stats["alive_hosts"] = 0;
    stats["total_ports"] = 0;
    stats["open_ports"] = 0;
    stats["vulnerable_hosts"] = 0;
    
    for (const auto& host : topology_.hosts) {
        if (host.alive) stats["alive_hosts"]++;
        
        for (const auto& port : host.ports) {
            stats["total_ports"]++;
            if (port.open) stats["open_ports"]++;
            if (port.vulnerable) {
                stats["vulnerable_hosts"]++;
                break;
            }
        }
    }
    
    return stats;
}

} // namespace security
