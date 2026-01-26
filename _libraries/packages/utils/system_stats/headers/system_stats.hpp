#pragma once
#include <string>
#include <vector>

namespace utils {
namespace system_stats {

struct DiskInfo {
    std::string mount_point;
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t available_bytes;
};

struct MemoryInfo {
    uint64_t total_ram;
    uint64_t available_ram;
};

struct CPUInfo {
    int cores;
    double usage_percent;
    std::string model;
};

struct GPUInfo {
    std::string name;
    double usage_percent;
    uint64_t memory_total;
    uint64_t memory_used;
};

std::vector<DiskInfo> get_disk_info();
MemoryInfo get_memory_info();
CPUInfo get_cpu_info();
std::vector<GPUInfo> get_gpu_info();

} // namespace system_stats
} // namespace utils
