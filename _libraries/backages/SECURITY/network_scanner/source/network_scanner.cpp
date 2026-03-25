#include "../headers/network_scanner.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <numeric>

// Platform-specific socket headers
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <poll.h>
  #include <netdb.h>
  #include <errno.h>
#endif

namespace security {

namespace {

#ifdef _WIN32
using socket_handle_t = SOCKET;
constexpr socket_handle_t invalid_socket_handle = INVALID_SOCKET;

void ensure_winsock_initialized() {
    static std::once_flag winsock_once;
    static bool winsock_ready = false;

    std::call_once(winsock_once, []() {
        WSADATA wsa_data;
        winsock_ready = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
    });

    if (!winsock_ready) {
        throw std::runtime_error("failed to initialize Winsock");
    }
}
#else
using socket_handle_t = int;
constexpr socket_handle_t invalid_socket_handle = -1;

void ensure_winsock_initialized() {}
#endif

} // namespace

// ── Helper functions ───────────────────────────────────────────────

std::vector<std::string> parse_network_range(const std::string& range) {
    std::vector<std::string> hosts;

    // Support CIDR notation (e.g., "192.168.1.0/24")
    auto slash_pos = range.find('/');
    if (slash_pos != std::string::npos) {
        std::string base_ip = range.substr(0, slash_pos);
        int prefix = std::stoi(range.substr(slash_pos + 1));
        uint32_t base = string_to_ip(base_ip);
        uint32_t mask = prefix == 0 ? 0 : (~0u << (32 - prefix));
        uint32_t network = base & mask;
        uint32_t broadcast = network | ~mask;

        for (uint32_t ip = network + 1; ip < broadcast; ++ip) {
            hosts.push_back(ip_to_string(ip));
        }
        return hosts;
    }

    // Support range notation (e.g., "192.168.1.1-254")
    auto dash_pos = range.find('-');
    if (dash_pos != std::string::npos) {
        // Find last dot
        auto last_dot = range.rfind('.', dash_pos);
        if (last_dot != std::string::npos) {
            std::string prefix = range.substr(0, last_dot + 1);
            int start = std::stoi(range.substr(last_dot + 1, dash_pos - last_dot - 1));
            int end = std::stoi(range.substr(dash_pos + 1));
            for (int i = start; i <= end; ++i) {
                hosts.push_back(prefix + std::to_string(i));
            }
            return hosts;
        }
    }

    // Single host
    hosts.push_back(range);
    return hosts;
}

std::string ip_to_string(uint32_t ip) {
    return std::to_string((ip >> 24) & 0xFF) + "." +
           std::to_string((ip >> 16) & 0xFF) + "." +
           std::to_string((ip >> 8) & 0xFF) + "." +
           std::to_string(ip & 0xFF);
}

uint32_t string_to_ip(const std::string& ip) {
    uint32_t result = 0;
    int shift = 24;
    std::istringstream ss(ip);
    std::string octet;
    while (std::getline(ss, octet, '.')) {
        result |= (static_cast<uint32_t>(std::stoi(octet)) << shift);
        shift -= 8;
    }
    return result;
}

// ── NetworkScanner ─────────────────────────────────────────────────

NetworkScanner::NetworkScanner(const ScanConfig& config)
    : config_(config) {}

static void close_socket(socket_handle_t fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

bool NetworkScanner::ping_host(const std::string& ip) {
    // Try connecting to common ports as a ping substitute
    // (raw ICMP requires root privileges)
    static const int ping_ports[] = {80, 443, 22, 21};
    for (int port : ping_ports) {
        if (scan_tcp_port(ip, port)) return true;
    }
    return false;
}

std::vector<std::string> NetworkScanner::discover_hosts(const std::string& network_range) {
    auto all_hosts = parse_network_range(network_range);
    std::vector<std::string> alive;
    for (auto& ip : all_hosts) {
        if (ping_host(ip)) {
            alive.push_back(ip);
            if (config_.verbose)
                std::cout << "[+] Host alive: " << ip << "\n";
        }
    }
    return alive;
}

bool NetworkScanner::scan_tcp_port(const std::string& ip, int port) {
    ensure_winsock_initialized();

    socket_handle_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == invalid_socket_handle) return false;

    // Set non-blocking
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    // Wait for connection with timeout
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv;
    tv.tv_sec = config_.timeout_ms / 1000;
    tv.tv_usec = (config_.timeout_ms % 1000) * 1000;
    int result = select(sock + 1, nullptr, &fds, nullptr, &tv);
#else
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLOUT;
    int result = poll(&pfd, 1, config_.timeout_ms);
#endif

    bool open = false;
    if (result > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
        open = (error == 0);
    }

    close_socket(sock);
    return open;
}

bool NetworkScanner::scan_udp_port(const std::string& ip, int port) {
    ensure_winsock_initialized();

    socket_handle_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == invalid_socket_handle) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    const char probe[] = "\x00";
    sendto(sock, probe, 1, 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    // Brief wait for ICMP unreachable
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    int result = select(sock + 1, &fds, nullptr, nullptr, &tv);
#else
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;
    int result = poll(&pfd, 1, 500);
#endif

    close_socket(sock);
    // If no response, port might be open (UDP is unreliable)
    return result <= 0;
}

std::vector<Port> NetworkScanner::scan_ports(const std::string& ip) {
    std::vector<Port> ports;
    for (int p : config_.ports_to_scan) {
        Port port;
        port.number = p;
        port.open = false;
        port.vulnerable = false;

        if (config_.scan_tcp) {
            port.protocol = "tcp";
            port.open = scan_tcp_port(ip, p);
        }

        if (!port.open && config_.scan_udp) {
            port.protocol = "udp";
            port.open = scan_udp_port(ip, p);
        }

        if (port.open) {
            port.service = detect_service(ip, p, port.protocol);
            ports.push_back(port);
        }
    }
    return ports;
}

std::string NetworkScanner::detect_service(const std::string& /*ip*/, int port,
                                           const std::string& /*protocol*/) {
    // Well-known port mapping
    static const std::map<int, std::string> services = {
        {21, "ftp"}, {22, "ssh"}, {23, "telnet"}, {25, "smtp"},
        {53, "dns"}, {80, "http"}, {110, "pop3"}, {143, "imap"},
        {443, "https"}, {445, "smb"}, {3306, "mysql"},
        {3389, "rdp"}, {5432, "postgresql"}, {8080, "http-proxy"},
        {8443, "https-alt"}, {9000, "custom"}, {9001, "custom"}, {9002, "custom"},
    };
    auto it = services.find(port);
    return it != services.end() ? it->second : "unknown";
}

std::string NetworkScanner::guess_os(const Host& host) {
    // Simple heuristic based on open ports
    bool has_smb = false, has_rdp = false, has_ssh = false;
    for (auto& p : host.ports) {
        if (p.number == 445) has_smb = true;
        if (p.number == 3389) has_rdp = true;
        if (p.number == 22) has_ssh = true;
    }
    if (has_rdp || (has_smb && !has_ssh)) return "Windows";
    if (has_ssh) return "Linux/Unix";
    return "Unknown";
}

void NetworkScanner::fuzz_service(Host& host, Port& port) {
    if (!config_.perform_fuzzing) return;
    FuzzConfig fc;
    fc.max_iterations = 50;
    fc.timeout_ms = config_.timeout_ms;
    NetworkFuzzer nf(host.ip, port.number, fc);
    nf.fuzz_tcp();
    auto stats = nf.get_statistics();
    if (stats["crashes"] > 0 || stats["exceptions"] > 0) {
        port.vulnerable = true;
        port.vulnerabilities.push_back("Possible crash vulnerability on port " +
                                       std::to_string(port.number));
    }
}

void NetworkScanner::scan_network(const std::string& network_range) {
    topology_.hosts.clear();
    topology_.connections.clear();

    auto alive_ips = discover_hosts(network_range);
    for (auto& ip : alive_ips) {
        scan_single_host(ip);
    }
    detect_connections();
}

void NetworkScanner::scan_single_host(const std::string& ip) {
    Host host;
    host.ip = ip;
    host.alive = true;

    auto start = std::chrono::steady_clock::now();
    host.ports = scan_ports(ip);
    auto end = std::chrono::steady_clock::now();
    host.response_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    host.os_guess = guess_os(host);

    // Resolve hostname
    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
    char hbuf[NI_MAXHOST];
    if (getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa),
                    hbuf, sizeof(hbuf), nullptr, 0, NI_NAMEREQD) == 0) {
        host.hostname = hbuf;
    } else {
        host.hostname = ip;
    }

    if (config_.perform_fuzzing) {
        for (auto& port : host.ports) {
            fuzz_service(host, port);
        }
    }

    topology_.hosts.push_back(host);
}

std::vector<Host> NetworkScanner::get_alive_hosts() const {
    std::vector<Host> alive;
    for (auto& h : topology_.hosts)
        if (h.alive) alive.push_back(h);
    return alive;
}

std::vector<Host> NetworkScanner::get_vulnerable_hosts() const {
    std::vector<Host> vuln;
    for (auto& h : topology_.hosts) {
        for (auto& p : h.ports) {
            if (p.vulnerable) { vuln.push_back(h); break; }
        }
    }
    return vuln;
}

void NetworkScanner::detect_connections() {
    // Simple: hosts on the same subnet are "connected"
    for (size_t i = 0; i < topology_.hosts.size(); ++i) {
        for (size_t j = i + 1; j < topology_.hosts.size(); ++j) {
            topology_.connections[topology_.hosts[i].ip].push_back(topology_.hosts[j].ip);
            topology_.connections[topology_.hosts[j].ip].push_back(topology_.hosts[i].ip);
        }
    }
}

std::string NetworkScanner::generate_dot_graph() const {
    std::ostringstream os;
    os << "graph network {\n";
    os << "  rankdir=LR;\n";
    for (auto& h : topology_.hosts) {
        os << "  \"" << h.ip << "\" [label=\"" << h.hostname << "\\n"
           << h.ip << "\\n" << h.os_guess << "\"];\n";
    }
    for (auto& [ip, conns] : topology_.connections) {
        for (auto& c : conns) {
            if (ip < c) // avoid duplicates
                os << "  \"" << ip << "\" -- \"" << c << "\";\n";
        }
    }
    os << "}\n";
    return os.str();
}

void NetworkScanner::print_results() const {
    std::cout << "\n=== Network Scan Results ===\n\n";
    for (auto& h : topology_.hosts) {
        std::cout << "Host: " << h.hostname << " (" << h.ip << ")\n"
                  << "  OS: " << h.os_guess
                  << " | Response: " << std::fixed << std::setprecision(1)
                  << h.response_time_ms << "ms\n"
                  << "  Open ports:\n";
        for (auto& p : h.ports) {
            std::cout << "    " << p.number << "/" << p.protocol
                      << " (" << p.service << ")";
            if (p.vulnerable) std::cout << " [VULNERABLE]";
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

void NetworkScanner::print_topology_ascii() const {
    std::cout << "\n=== Network Topology ===\n\n";
    for (auto& h : topology_.hosts) {
        std::cout << "[" << h.ip << "] -- ";
        auto it = topology_.connections.find(h.ip);
        if (it != topology_.connections.end()) {
            for (size_t i = 0; i < it->second.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << it->second[i];
            }
        }
        std::cout << "\n";
    }
}

void NetworkScanner::export_topology(const std::string& filename,
                                     const std::string& format) const {
    std::ofstream f(filename);
    if (!f) return;
    if (format == "dot") {
        f << generate_dot_graph();
    } else {
        // JSON-like output
        f << "{\n  \"hosts\": [\n";
        for (size_t i = 0; i < topology_.hosts.size(); ++i) {
            auto& h = topology_.hosts[i];
            f << "    {\"ip\": \"" << h.ip << "\", \"hostname\": \"" << h.hostname
              << "\", \"os\": \"" << h.os_guess << "\", \"ports\": [";
            for (size_t j = 0; j < h.ports.size(); ++j) {
                if (j > 0) f << ", ";
                f << h.ports[j].number;
            }
            f << "]}";
            if (i + 1 < topology_.hosts.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
    }
}

std::map<std::string, int> NetworkScanner::get_statistics() const {
    std::map<std::string, int> stats;
    stats["total_hosts"] = static_cast<int>(topology_.hosts.size());
    stats["alive_hosts"] = static_cast<int>(get_alive_hosts().size());
    stats["vulnerable_hosts"] = static_cast<int>(get_vulnerable_hosts().size());
    int total_ports = 0;
    for (auto& h : topology_.hosts) total_ports += static_cast<int>(h.ports.size());
    stats["total_open_ports"] = total_ports;
    return stats;
}

} // namespace security
