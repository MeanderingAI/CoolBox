#include "system/host_scanner.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <sys/sysctl.h>
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <mach/mach.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <libproc.h>

namespace host {
namespace scanner {

// Utility functions
std::string format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
}

std::string format_duration(std::chrono::seconds duration) {
    auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count() % 24;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    
    std::stringstream ss;
    if (days > 0) ss << days << "d ";
    if (hours > 0 || days > 0) ss << hours << "h ";
    ss << minutes << "m";
    return ss.str();
}

std::vector<int> get_common_ports() {
    return {
        21, 22, 23, 25, 53, 80, 110, 143, 443, 465, 587, 993, 995,
        1433, 3306, 3389, 5432, 5900, 6379, 8080, 8443, 27017
    };
}

// SystemScanner implementation
SystemScanner::SystemScanner() : monitoring_(false) {}

CPUInfo SystemScanner::get_cpu_info() {
    CPUInfo info;
    
    // Get CPU model
    size_t size = 256;
    char brand[256];
    sysctlbyname("machdep.cpu.brand_string", brand, &size, nullptr, 0);
    info.model = brand;
    
    // Get core count
    int cores, threads;
    size = sizeof(cores);
    sysctlbyname("hw.physicalcpu", &cores, &size, nullptr, 0);
    sysctlbyname("hw.logicalcpu", &threads, &size, nullptr, 0);
    info.cores = cores;
    info.threads = threads;
    
    // Get CPU usage (simplified)
    host_cpu_load_info_data_t cpu_info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                        (host_info_t)&cpu_info, &count) == KERN_SUCCESS) {
        
        unsigned long long total_ticks = 0;
        for (int i = 0; i < CPU_STATE_MAX; i++) {
            total_ticks += cpu_info.cpu_ticks[i];
        }
        
        if (total_ticks > 0) {
            info.user_time_percent = (cpu_info.cpu_ticks[CPU_STATE_USER] * 100.0) / total_ticks;
            info.system_time_percent = (cpu_info.cpu_ticks[CPU_STATE_SYSTEM] * 100.0) / total_ticks;
            info.idle_percent = (cpu_info.cpu_ticks[CPU_STATE_IDLE] * 100.0) / total_ticks;
            info.usage_percent = 100.0 - info.idle_percent;
        }
    }
    
    return info;
}

MemoryInfo SystemScanner::get_memory_info() {
    MemoryInfo info;
    
    // Get total memory
    int64_t mem_size;
    size_t size = sizeof(mem_size);
    sysctlbyname("hw.memsize", &mem_size, &size, nullptr, 0);
    info.total_bytes = mem_size;
    
    // Get memory usage
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        
        vm_size_t page_size;
        host_page_size(mach_host_self(), &page_size);
        
        size_t used = (vm_stats.active_count + vm_stats.wire_count) * page_size;
        size_t free = vm_stats.free_count * page_size;
        
        info.used_bytes = used;
        info.available_bytes = free;
        info.usage_percent = (used * 100.0) / mem_size;
    }
    
    // Swap info (simplified for macOS)
    info.swap_total_bytes = 0;
    info.swap_used_bytes = 0;
    info.swap_usage_percent = 0.0;
    
    return info;
}

std::vector<DiskInfo> SystemScanner::get_disk_info() {
    std::vector<DiskInfo> disks;
    
    // Get mounted filesystems
    struct statfs *mounts;
    int count = getmntinfo(&mounts, MNT_NOWAIT);
    
    for (int i = 0; i < count; i++) {
        // Skip special filesystems
        std::string fs_type = mounts[i].f_fstypename;
        if (fs_type == "devfs" || fs_type == "autofs") continue;
        
        DiskInfo disk;
        disk.mount_point = mounts[i].f_mntonname;
        disk.filesystem = mounts[i].f_fstypename;
        disk.total_bytes = mounts[i].f_blocks * mounts[i].f_bsize;
        disk.available_bytes = mounts[i].f_bavail * mounts[i].f_bsize;
        disk.used_bytes = disk.total_bytes - disk.available_bytes;
        disk.usage_percent = (disk.used_bytes * 100.0) / disk.total_bytes;
        
        disks.push_back(disk);
    }
    
    return disks;
}

std::vector<ProcessInfo> SystemScanner::get_top_processes(int limit) {
    std::vector<ProcessInfo> processes;
    
    // Get process list (simplified - would need full implementation)
    int pid_count = proc_listallpids(nullptr, 0);
    std::vector<int> pids(pid_count);
    proc_listallpids(pids.data(), pid_count * sizeof(int));
    
    for (int i = 0; i < std::min(limit, pid_count); i++) {
        ProcessInfo proc;
        proc.pid = pids[i];
        
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pids[i], pathbuf, sizeof(pathbuf)) > 0) {
            proc.name = pathbuf;
            // Extract just the filename
            size_t pos = proc.name.find_last_of('/');
            if (pos != std::string::npos) {
                proc.name = proc.name.substr(pos + 1);
            }
        } else {
            proc.name = "Unknown";
        }
        
        proc.user = "user";
        proc.cpu_percent = 0.0;
        proc.memory_bytes = 0;
        proc.state = "running";
        
        processes.push_back(proc);
    }
    
    return processes;
}

std::string SystemScanner::get_hostname() {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    return hostname;
}

std::string SystemScanner::get_os_name() {
    return "macOS";
}

std::string SystemScanner::get_os_version() {
    char version[256];
    size_t size = sizeof(version);
    sysctlbyname("kern.osproductversion", version, &size, nullptr, 0);
    return version;
}

std::string SystemScanner::get_kernel_version() {
    char version[256];
    size_t size = sizeof(version);
    sysctlbyname("kern.osrelease", version, &size, nullptr, 0);
    return version;
}

std::chrono::system_clock::time_point SystemScanner::get_boot_time() {
    struct timeval boot_time;
    size_t size = sizeof(boot_time);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    sysctl(mib, 2, &boot_time, &size, nullptr, 0);
    
    return std::chrono::system_clock::from_time_t(boot_time.tv_sec);
}

std::chrono::seconds SystemScanner::get_uptime() {
    auto boot_time = get_boot_time();
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - boot_time);
}

// NetworkScanner implementation
NetworkScanner::NetworkScanner() 
    : max_threads_(10),
      scan_timeout_(std::chrono::milliseconds(1000)) {
}

std::vector<NetworkInterface> NetworkScanner::get_network_interfaces() {
    std::vector<NetworkInterface> interfaces;
    
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    
    std::map<std::string, NetworkInterface> iface_map;
    
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            std::string name = tmp->ifa_name;
            
            if (iface_map.find(name) == iface_map.end()) {
                NetworkInterface iface;
                iface.name = name;
                iface.status = (tmp->ifa_flags & IFF_UP) ? "up" : "down";
                iface.mac_address = "00:00:00:00:00:00";  // Simplified
                iface.bytes_sent = 0;
                iface.bytes_received = 0;
                iface.packets_sent = 0;
                iface.packets_received = 0;
                iface.errors_in = 0;
                iface.errors_out = 0;
                iface_map[name] = iface;
            }
            
            // Get IP address
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in*)tmp->ifa_addr)->sin_addr, ip, sizeof(ip));
            iface_map[name].ip_addresses.push_back(ip);
        }
        tmp = tmp->ifa_next;
    }
    
    freeifaddrs(addrs);
    
    for (auto& [name, iface] : iface_map) {
        interfaces.push_back(iface);
    }
    
    return interfaces;
}

std::vector<NetworkConnection> NetworkScanner::get_active_connections() {
    std::vector<NetworkConnection> connections;
    
    // Simplified - would use netstat or parse /proc/net/tcp on Linux
    // macOS implementation would use lsof or similar
    
    return connections;
}

std::vector<NetworkConnection> NetworkScanner::get_listening_ports() {
    return get_active_connections();  // Filter by state
}

PortScanResult NetworkScanner::scan_port(const std::string& host, int port, std::chrono::milliseconds timeout) {
    PortScanResult result;
    result.port = port;
    result.open = false;
    result.service = get_service_name(port);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        result.response_time = std::chrono::milliseconds(0);
        return result;
    }
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    // Resolve host
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    
    // Try to connect
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        result.open = true;
        
        // Try to read banner
        char buffer[1024] = {0};
        recv(sock, buffer, sizeof(buffer) - 1, 0);
        result.banner = buffer;
    }
    
    close(sock);
    
    auto end = std::chrono::high_resolution_clock::now();
    result.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

std::vector<PortScanResult> NetworkScanner::scan_ports(const std::string& host, const std::vector<int>& ports) {
    std::vector<PortScanResult> results;
    
    for (int port : ports) {
        results.push_back(scan_port(host, port, scan_timeout_));
    }
    
    return results;
}

std::vector<PortScanResult> NetworkScanner::scan_common_ports(const std::string& host) {
    return scan_ports(host, get_common_ports());
}

bool NetworkScanner::ping(const std::string& host, std::chrono::milliseconds timeout) {
    // Simplified ping using connect
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);  // Try common port
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    
    bool reachable = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);
    
    return reachable;
}

std::vector<NetworkDevice> NetworkScanner::discover_devices(const std::string& network_range) {
    std::vector<NetworkDevice> devices;
    
    // Simplified discovery - scan local subnet
    std::string local_ip = get_local_ip();
    
    // Parse local IP and scan /24 subnet
    size_t last_dot = local_ip.find_last_of('.');
    if (last_dot != std::string::npos) {
        std::string subnet = local_ip.substr(0, last_dot);
        
        for (int i = 1; i < 255; i++) {
            std::string ip = subnet + "." + std::to_string(i);
            
            if (ping(ip, std::chrono::milliseconds(100))) {
                NetworkDevice device;
                device.ip_address = ip;
                device.is_reachable = true;
                device.ping_time = std::chrono::milliseconds(0);
                device.mac_address = "Unknown";
                device.hostname = ip;
                device.vendor = "Unknown";
                
                devices.push_back(device);
            }
        }
    }
    
    return devices;
}

std::string NetworkScanner::get_local_ip() {
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    
    std::string ip;
    
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            std::string name = tmp->ifa_name;
            if (name != "lo0") {  // Skip loopback
                char addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((struct sockaddr_in*)tmp->ifa_addr)->sin_addr, addr, sizeof(addr));
                ip = addr;
                break;
            }
        }
        tmp = tmp->ifa_next;
    }
    
    freeifaddrs(addrs);
    return ip;
}

std::string NetworkScanner::get_public_ip() {
    return "N/A";  // Would require external service
}

std::string NetworkScanner::get_default_gateway() {
    return "N/A";  // Would parse routing table
}

std::string NetworkScanner::get_service_name(int port) {
    static std::map<int, std::string> services = {
        {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"}, 
        {53, "DNS"}, {80, "HTTP"}, {110, "POP3"}, {143, "IMAP"},
        {443, "HTTPS"}, {3306, "MySQL"}, {5432, "PostgreSQL"},
        {6379, "Redis"}, {8080, "HTTP-Alt"}, {27017, "MongoDB"}
    };
    
    auto it = services.find(port);
    return it != services.end() ? it->second : "Unknown";
}

std::string NetworkScanner::get_mac_vendor(const std::string& mac) {
    return "Unknown";  // Would require OUI database
}

// HostScanner implementation
HostScanner::HostScanner() {}

HostScanner::ScanReport HostScanner::generate_report() {
    ScanReport report;
    
    report.scan_time = std::chrono::system_clock::now();
    report.hostname = system_scanner_.get_hostname();
    report.os_info = system_scanner_.get_os_name() + " " + system_scanner_.get_os_version();
    report.cpu = system_scanner_.get_cpu_info();
    report.memory = system_scanner_.get_memory_info();
    report.disks = system_scanner_.get_disk_info();
    report.interfaces = network_scanner_.get_network_interfaces();
    report.connections = network_scanner_.get_active_connections();
    report.top_processes = system_scanner_.get_top_processes(10);
    
    return report;
}

std::string HostScanner::report_to_json(const ScanReport& report) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"hostname\": \"" << report.hostname << "\",\n";
    ss << "  \"os\": \"" << report.os_info << "\",\n";
    ss << "  \"cpu_usage\": " << report.cpu.usage_percent << ",\n";
    ss << "  \"memory_usage\": " << report.memory.usage_percent << ",\n";
    ss << "  \"interfaces\": " << report.interfaces.size() << "\n";
    ss << "}\n";
    return ss.str();
}

} // namespace scanner
} // namespace host
