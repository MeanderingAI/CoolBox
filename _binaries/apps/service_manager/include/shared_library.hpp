
#ifndef SERVICE_MANAGER_SHARED_LIBRARY_HPP
#define SERVICE_MANAGER_SHARED_LIBRARY_HPP

#include <string>
#include <ctime>
#include <vector>
#include <sys/stat.h>
#include <dlfcn.h>

#include "unix_commands/unix_commands.hpp"

struct SharedLibrary {
    std::string name;
    std::string path;
    std::string target_name;
    size_t file_size;
    time_t last_modified;
    std::string make_command;
    std::string lib_name;
    std::string version;
    std::string description;
    std::string author;
    bool has_metadata = false;
};

inline std::vector<SharedLibrary> scan_libraries(const std::string& workspace_path) {
    std::vector<SharedLibrary> libraries;
    
    std::string cmd = UnixCommands::FIND_SHARED_LIBS_CMD(workspace_path);
    std::vector<std::string> lib_paths = UnixCommands::run(cmd);
    for (const auto& lib_path : lib_paths) {
        SharedLibrary lib;
        lib.path = lib_path;
        size_t last_slash = lib_path.rfind('/');
        lib.name = (last_slash != std::string::npos) ? lib_path.substr(last_slash + 1) : lib_path;
        struct stat st;
        if (stat(lib_path.c_str(), &st) == 0) {
            lib.file_size = st.st_size;
            lib.last_modified = st.st_mtime;
        }
        std::string relative_path = lib_path;
        size_t src_pos = relative_path.find("/libraries/src/");
        if (src_pos != std::string::npos) {
            relative_path = relative_path.substr(src_pos + 5);
            size_t first_slash = relative_path.find('/');
            if (first_slash != std::string::npos) {
                lib.target_name = relative_path.substr(0, first_slash);
            }
        }
        std::string clean_name = lib.name;
        if (clean_name.substr(0, 3) == "lib") clean_name = clean_name.substr(3);
        size_t dot_pos = clean_name.find('.');
        if (dot_pos != std::string::npos) clean_name = clean_name.substr(0, dot_pos);
        if (lib.target_name.empty()) lib.target_name = clean_name;
        lib.make_command = "cmake --build build --target " + lib.target_name + " -j8";
        void* handle = dlopen(lib_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (handle) {
            typedef const char* (*get_string_func)();
            // ...existing code...
        }
        libraries.push_back(lib);
    }
    return libraries;
}

#endif // SERVICE_MANAGER_SHARED_LIBRARY_HPP
