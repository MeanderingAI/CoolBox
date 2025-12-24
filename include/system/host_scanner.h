#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <functional>

namespace host {
namespace scanner {

// System resource information
struct CPUInfo {
    std::string model;
    int cores;
    int threads;
    double usage_percent;
    double user_time_percent;
    double system_time_percent;
    double idle_percent;
    std::vector<double> per_core_usage;
};

struct MemoryInfo {
    size_t total_bytes;
    size_t available_bytes;
    size_t used_bytes;
    double usage_percent;
    size_t swap_total_bytes;
    size_t swap_used_bytes;
    double swap_usage_percent;
};

struct DiskInfo {
    std::string mount_point;
    std::string filesystem;
    size_t total_bytes;
    size_t used_bytes;
    size_t available_bytes;
    double usage_percent;
};

struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
    double cpu_percent;
    size_t memory_bytes;
    std::string state;
};

// Network information
struct NetworkInterface {
    std::string name;
    std::string mac_address;
    std::vector<std::string> ip_addresses;
    std::string status;  // up, down
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t errors_in;
    uint64_t errors_out;
};

struct NetworkConnection {
    std::string protocol;  // TCP, UDP
    std::string local_address;
    int local_port;
    std::string remote_address;
    int remote_port;
    std::string state;  // ESTABLISHED, LISTEN, etc.
    int pid;
    std::string process_name;
};

struct PortScanResult {
    int port;
    bool open;
    std::string service;
    std::string banner;
    std::chrono::milliseconds response_time;
};

struct NetworkDevice {
    std::string ip_address;
    std::string mac_address;
    std::string hostname;
    bool is_reachable;
    std::chrono::milliseconds ping_time;
    std::vector<int> open_ports;
    std::string vendor;
};

// System scanner
class SystemScanner {
public:
    SystemScanner();
    ~SystemScanner() = default;
    
    // System information
    CPUInfo get_cpu_info();
    MemoryInfo get_memory_info();
    std::vector<DiskInfo> get_disk_info();
    std::vector<ProcessInfo> get_top_processes(int limit = 10);
    
    // System details
    std::string get_hostname();
    std::string get_os_name();
    std::string get_os_version();
    std::string get_kernel_version();
    std::chrono::system_clock::time_point get_boot_time();
    std::chrono::seconds get_uptime();
    
    // Continuous monitoring
    void start_monitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    void stop_monitoring();
    bool is_monitoring() const { return monitoring_; }
    
    // Callbacks for updates
    void set_cpu_callback(std::function<void(const CPUInfo&)> callback) { cpu_callback_ = callback; }
    void set_memory_callback(std::function<void(const MemoryInfo&)> callback) { memory_callback_ = callback; }
    
private:
    bool monitoring_;
    std::function<void(const CPUInfo&)> cpu_callback_;
    std::function<void(const MemoryInfo&)> memory_callback_;
    
    void monitoring_thread();
};

// Network scanner
class NetworkScanner {
public:
    NetworkScanner();
    ~NetworkScanner() = default;
    
    // Network interfaces
    std::vector<NetworkInterface> get_network_interfaces();
    NetworkInterface get_interface_info(const std::string& interface_name);
    
    // Active connections
    std::vector<NetworkConnection> get_active_connections();
    std::vector<NetworkConnection> get_listening_ports();
    
    // Port scanning
    PortScanResult scan_port(const std::string& host, int port, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    std::vector<PortScanResult> scan_ports(const std::string& host, const std::vector<int>& ports);
    std::vector<PortScanResult> scan_common_ports(const std::string& host);
    
    // Network discovery
    std::vector<NetworkDevice> discover_devices(const std::string& network_range = "");
    bool ping(const std::string& host, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    
    // Utilities
    std::string get_local_ip();
    std::string get_public_ip();
    std::string get_default_gateway();
    
    // Configuration
    void set_max_threads(int threads) { max_threads_ = threads; }
    void set_scan_timeout(std::chrono::milliseconds timeout) { scan_timeout_ = timeout; }
    
private:
    int max_threads_;
    std::chrono::milliseconds scan_timeout_;
    
    std::string get_service_name(int port);
    std::string get_mac_vendor(const std::string& mac);
};

// Combined scanner for comprehensive system and network analysis
class HostScanner {
public:
    HostScanner();
    ~HostScanner() = default;
    
    struct ScanReport {
        std::chrono::system_clock::time_point scan_time;
        std::string hostname;
        std::string os_info;
        CPUInfo cpu;
        MemoryInfo memory;
        std::vector<DiskInfo> disks;
        std::vector<NetworkInterface> interfaces;
        std::vector<NetworkConnection> connections;
        std::vector<ProcessInfo> top_processes;
        std::vector<NetworkDevice> network_devices;
    };
    
    ScanReport generate_report();
    void save_report(const ScanReport& report, const std::string& filepath);
    std::string report_to_json(const ScanReport& report);
    std::string report_to_html(const ScanReport& report);
    
    SystemScanner& system_scanner() { return system_scanner_; }
    NetworkScanner& network_scanner() { return network_scanner_; }
    
private:
    SystemScanner system_scanner_;
    NetworkScanner network_scanner_;
};

// Utility functions
std::string format_bytes(size_t bytes);
std::string format_duration(std::chrono::seconds duration);
std::vector<int> get_common_ports();

} // namespace scanner
} // namespace host
