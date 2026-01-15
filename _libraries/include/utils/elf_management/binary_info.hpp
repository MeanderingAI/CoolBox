#pragma once
#include <string>
#include <ctime>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

namespace utils {
namespace elf_management {

struct BinaryInfo {
    std::string name;
    std::string path;
    size_t size;
    time_t last_modified;
    std::string type;
};

inline std::vector<BinaryInfo> scan_binaries(const std::string& workspace_path) {
    std::vector<BinaryInfo> bins;
    std::string bin_dir = workspace_path + "/build/bin";
    DIR* dir = opendir(bin_dir.c_str());
    if (!dir) return bins;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) continue;
        std::string name = entry->d_name;
        std::string path = bin_dir + "/" + name;
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            std::string type = "tool";
            if (name.find("test") != std::string::npos) type = "test";
            else if (name.find("demo") != std::string::npos) type = "demo";
            bins.push_back(BinaryInfo{name, path, (size_t)st.st_size, st.st_mtime, type});
        }
    }
    closedir(dir);
    return bins;
}

} // namespace elf_management
} // namespace utils
