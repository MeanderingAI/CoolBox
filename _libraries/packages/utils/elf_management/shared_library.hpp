#pragma once
#include <string>
#include <dlfcn.h>
#include <vector>
#include <filesystem>

namespace utils {
namespace elf_management {

class SharedLibrary {
public:
    SharedLibrary(const std::string& path) : handle_(nullptr) {
        handle_ = dlopen(path.c_str(), RTLD_LAZY);
    }
    ~SharedLibrary() {
        if (handle_) dlclose(handle_);
    }
    bool is_loaded() const { return handle_ != nullptr; }
    void* get_symbol(const std::string& name) {
        return handle_ ? dlsym(handle_, name.c_str()) : nullptr;
    }
private:
    void* handle_;
};

struct LibraryInfo {
    std::string name;
    std::string path;
    size_t size;
    std::string type;
    std::string target_name;
    size_t file_size;
    time_t last_modified;
    std::string make_command;
    std::string lib_name;
    std::string version;
    std::string description;
    std::string author;
    bool has_metadata;
};

inline std::vector<LibraryInfo> scan_libraries(const std::string& workspace_path) {
    std::vector<LibraryInfo> libs;
    std::string lib_dir = workspace_path + "/build/lib";
    for (const auto& entry : std::filesystem::directory_iterator(lib_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        std::string path = entry.path().string();
        size_t size = entry.file_size();
        std::string type = "shared";
        if (name.find("static") != std::string::npos) type = "static";
        // Metadata extraction (stubbed for now)
        std::string target_name = name;
        size_t file_size = size;
        time_t last_modified = std::filesystem::last_write_time(entry).time_since_epoch().count();
        std::string make_command = "make " + name;
        std::string lib_name = name;
        std::string version = "1.0.0";
        std::string description = "";
        std::string author = "";
        bool has_metadata = false;
        libs.push_back(LibraryInfo{name, path, size, type, target_name, file_size, last_modified, make_command, lib_name, version, description, author, has_metadata});
    }
    return libs;
}

} // namespace elf_management
} // namespace utils
