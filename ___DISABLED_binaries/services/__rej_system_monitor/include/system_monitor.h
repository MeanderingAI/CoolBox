#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace services {

struct SystemMetrics {
    double cpu_usage;           // Percentage
    double memory_usage;        // Percentage
    double memory_total_mb;     // MB
    double memory_used_mb;      // MB
    double disk_usage;          // Percentage
    double disk_total_gb;       // GB
    double disk_used_gb;        // GB
    double network_rx_mbps;     // Mbps
    double network_tx_mbps;     // Mbps
    long network_rx_bytes;      // Total bytes received
    long network_tx_bytes;      // Total bytes transmitted
    int process_count;          // Number of processes
    std::string uptime;         // System uptime
    std::string timestamp;
};

struct NetworkInterface {
    std::string name;
    long bytes_received;
    long bytes_transmitted;
    std::chrono::steady_clock::time_point last_check;
};

class SystemMonitor {
public:
    SystemMonitor();
    
    // Get current metrics
    SystemMetrics get_metrics();
    
    // Get historical data (last N samples)
    std::vector<SystemMetrics> get_history(int limit = 10);
    
    // Update metrics
    void update();
    
private:
    double get_cpu_usage();
    double get_memory_usage(double& total_mb, double& used_mb);
    double get_disk_usage(double& total_gb, double& used_gb);
    void get_network_stats(double& rx_mbps, double& tx_mbps, long& rx_bytes, long& tx_bytes);
    int get_process_count();
    std::string get_uptime();
    std::string get_timestamp();
    
    std::vector<SystemMetrics> history_;
    std::map<std::string, NetworkInterface> network_interfaces_;
    std::chrono::steady_clock::time_point last_cpu_check_;
    long last_cpu_time_;
};

} // namespace services
