#include "services/system_monitor/system_monitor.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/statvfs.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/mach.h>

namespace services {

SystemMonitor::SystemMonitor() 
    : last_cpu_time_(0) {
    last_cpu_check_ = std::chrono::steady_clock::now();
}

SystemMetrics SystemMonitor::get_metrics() {
    SystemMetrics metrics;
    
    metrics.cpu_usage = get_cpu_usage();
    metrics.memory_usage = get_memory_usage(metrics.memory_total_mb, metrics.memory_used_mb);
    metrics.disk_usage = get_disk_usage(metrics.disk_total_gb, metrics.disk_used_gb);
    get_network_stats(metrics.network_rx_mbps, metrics.network_tx_mbps, 
                      metrics.network_rx_bytes, metrics.network_tx_bytes);
    metrics.process_count = get_process_count();
    metrics.uptime = get_uptime();
    metrics.timestamp = get_timestamp();
    
    return metrics;
}

std::vector<SystemMetrics> SystemMonitor::get_history(int limit) {
    if (history_.size() <= limit) {
        return history_;
    }
    return std::vector<SystemMetrics>(history_.end() - limit, history_.end());
}

void SystemMonitor::update() {
    SystemMetrics metrics = get_metrics();
    history_.push_back(metrics);
    
    // Keep only last 100 samples
    if (history_.size() > 100) {
        history_.erase(history_.begin());
    }
}

double SystemMonitor::get_cpu_usage() {
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        unsigned long long total = 0;
        for (int i = 0; i < CPU_STATE_MAX; i++) {
            total += cpuinfo.cpu_ticks[i];
        }
        
        unsigned long long idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        
        if (last_cpu_time_ > 0) {
            unsigned long long total_diff = total - last_cpu_time_;
            if (total_diff > 0) {
                double usage = 100.0 * (1.0 - ((double)idle / (double)total_diff));
                last_cpu_time_ = total;
                return usage > 0 ? usage : 0.0;
            }
        }
        
        last_cpu_time_ = total;
        return 0.0;
    }
    
    return 0.0;
}

double SystemMonitor::get_memory_usage(double& total_mb, double& used_mb) {
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics64_data_t vm_stats;
    
    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    
    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO,
                                          (host_info64_t)&vm_stats, &count)) {
        
        long long free_memory = (int64_t)vm_stats.free_count * (int64_t)page_size;
        long long used_memory = ((int64_t)vm_stats.active_count +
                                 (int64_t)vm_stats.inactive_count +
                                 (int64_t)vm_stats.wire_count) * (int64_t)page_size;
        
        // Get total physical memory
        int mib[2] = {CTL_HW, HW_MEMSIZE};
        int64_t physical_memory;
        size_t length = sizeof(physical_memory);
        sysctl(mib, 2, &physical_memory, &length, NULL, 0);
        
        total_mb = physical_memory / (1024.0 * 1024.0);
        used_mb = used_memory / (1024.0 * 1024.0);
        
        return (used_mb / total_mb) * 100.0;
    }
    
    total_mb = 0;
    used_mb = 0;
    return 0.0;
}

double SystemMonitor::get_disk_usage(double& total_gb, double& used_gb) {
    struct statvfs stat;
    
    if (statvfs("/", &stat) == 0) {
        unsigned long long total = stat.f_blocks * stat.f_frsize;
        unsigned long long available = stat.f_bavail * stat.f_frsize;
        unsigned long long used = total - available;
        
        total_gb = total / (1024.0 * 1024.0 * 1024.0);
        used_gb = used / (1024.0 * 1024.0 * 1024.0);
        
        return (used_gb / total_gb) * 100.0;
    }
    
    total_gb = 0;
    used_gb = 0;
    return 0.0;
}

void SystemMonitor::get_network_stats(double& rx_mbps, double& tx_mbps, 
                                       long& rx_bytes, long& tx_bytes) {
    // Simplified network stats - in production, use proper system APIs
    // For macOS, we'd use sysctlbyname with "net.link.generic.system"
    rx_bytes = 0;
    tx_bytes = 0;
    rx_mbps = 0.0;
    tx_mbps = 0.0;
    
    // Read from /proc/net/dev on Linux, or use sysctl on macOS
    // For demo purposes, generate some sample data
    static long sample_rx = 1024 * 1024 * 100; // 100 MB
    static long sample_tx = 1024 * 1024 * 50;  // 50 MB
    
    sample_rx += (rand() % 1024) * 1024; // Add 0-1 MB
    sample_tx += (rand() % 512) * 1024;  // Add 0-512 KB
    
    rx_bytes = sample_rx;
    tx_bytes = sample_tx;
    
    // Calculate Mbps based on time delta
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_cpu_check_).count() / 1000.0;
    
    if (elapsed > 0) {
        rx_mbps = (rand() % 100) / 10.0; // 0-10 Mbps
        tx_mbps = (rand() % 50) / 10.0;  // 0-5 Mbps
    }
}

int SystemMonitor::get_process_count() {
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    
    if (sysctl(mib, 4, NULL, &size, NULL, 0) == 0) {
        return size / sizeof(struct kinfo_proc);
    }
    
    return 0;
}

std::string SystemMonitor::get_uptime() {
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) == 0) {
        time_t now = time(NULL);
        time_t uptime_seconds = now - boottime.tv_sec;
        
        int days = uptime_seconds / 86400;
        int hours = (uptime_seconds % 86400) / 3600;
        int minutes = (uptime_seconds % 3600) / 60;
        
        std::stringstream ss;
        if (days > 0) ss << days << "d ";
        ss << hours << "h " << minutes << "m";
        
        return ss.str();
    }
    
    return "Unknown";
}

std::string SystemMonitor::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace services
