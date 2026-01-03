/*
 * Utility to extract embedded documentation from shared libraries
 */

#include <iostream>
#include <string>
#include <dlfcn.h>

struct LibraryInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    bool has_metadata = false;
};

LibraryInfo extract_library_metadata(const std::string& dylib_path) {
    LibraryInfo info;
    
    // Open the library
    void* handle = dlopen(dylib_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        std::cerr << "Failed to load: " << dylib_path << " - " << dlerror() << std::endl;
        return info;
    }
    
    // Try to get metadata functions
    typedef const char* (*get_string_func)();
    
    auto get_name = (get_string_func)dlsym(handle, "get_library_name");
    auto get_version = (get_string_func)dlsym(handle, "get_library_version");
    auto get_desc = (get_string_func)dlsym(handle, "get_library_description");
    auto get_author = (get_string_func)dlsym(handle, "get_library_author");
    auto get_doc = (get_string_func)dlsym(handle, "get_library_doc");
    
    if (get_name && get_version && get_desc && get_author) {
        info.name = get_name();
        info.version = get_version();
        info.description = get_desc();
        info.author = get_author();
        info.has_metadata = true;
    } else if (get_doc) {
        info.description = get_doc();
        info.has_metadata = true;
    }
    
    dlclose(handle);
    return info;
}

// Usage example
int main() {
    auto info = extract_library_metadata("build/src/cache/libcache_server.dylib");
    
    if (info.has_metadata) {
        std::cout << "Library: " << info.name << std::endl;
        std::cout << "Version: " << info.version << std::endl;
        std::cout << "Description: " << info.description << std::endl;
        std::cout << "Author: " << info.author << std::endl;
    } else {
        std::cout << "No embedded metadata found" << std::endl;
    }
    
    return 0;
}
