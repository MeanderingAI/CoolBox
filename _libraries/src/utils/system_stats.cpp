#include "utils/system_stats/system_stats.hpp"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

namespace utils {
namespace system_stats {

std::vector<DiskInfo> get_disk_info() {
    std::vector<DiskInfo> disks;
    // Example: Use statvfs for root
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        DiskInfo info;
        info.mount_point = "/";
        info.total_bytes = stat.f_blocks * stat.f_frsize;
        info.available_bytes = stat.f_bavail * stat.f_frsize;
        info.used_bytes = info.total_bytes - info.available_bytes;
        disks.push_back(info);
    }
    // Extend: parse /etc/mtab or use getmntinfo for all mounts
    return disks;
}

MemoryInfo get_memory_info() {
    MemoryInfo info;
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    info.total_ram = pages * page_size;
#ifdef __APPLE__
    int mib[2];
    int64_t available_ram;
    size_t len = sizeof(available_ram);
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    sysctl(mib, 2, &available_ram, &len, NULL, 0);
    info.available_ram = available_ram;
#else
    info.available_ram = sysconf(_SC_AVPHYS_PAGES) * page_size;
#endif
    return info;
}

CPUInfo get_cpu_info() {
    CPUInfo info;
    info.cores = sysconf(_SC_NPROCESSORS_ONLN);
    info.model = "Unknown";
    info.usage_percent = 0.0; // Placeholder, needs /proc/stat parsing or sysctl
    // Extend: parse /proc/cpuinfo or use sysctlbyname
    return info;
}

std::vector<GPUInfo> get_gpu_info() {
    std::vector<GPUInfo> gpus;
    // Placeholder: GPU info requires platform-specific APIs (e.g., Metal, CUDA, OpenCL)
    // Example: No GPU detected
    return gpus;
}

} // namespace system_stats
} // namespace utils
