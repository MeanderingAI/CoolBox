#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace dj = dataformats::json;

namespace service_manager {

std::string handle_api_routes() {
    dj::Array routes_json;
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/api/libdocs").add("description", "List all available library documentation HTML index files").build());
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/api/libraries").add("description", "List all shared libraries and their metadata").build());
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/api/binaries").add("description", "List all generated binaries in build/bin").build());
    routes_json.push(dj::Builder().add("method", "POST").add("path", "/api/rebuild").add("description", "Rebuild a specific shared library by target name").build());
    routes_json.push(dj::Builder().add("method", "POST").add("path", "/api/docs-rebuild").add("description", "Regenerate all API documentation").build());
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/docs/*").add("description", "Serve generated documentation HTML files").build());
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/").add("description", "Service manager web UI").build());
    routes_json.push(dj::Builder().add("method", "GET").add("path", "/index").add("description", "Service manager web UI (index)").build());
    dj::Object resp_obj;
    resp_obj.set("routes", routes_json);
    return resp_obj.to_string();
}

#include "MISC/elf_management/binary_info.hpp"
#include <dirent.h>

std::string handle_api_binaries(const std::string& workspace_path) {
    auto bins = utils::elf_management::scan_binaries(workspace_path);
    dj::Array arr;
    for (const auto& bin : bins) {
        arr.push(
            dj::Builder()
                .add("name", bin.name)
                .add("path", bin.path)
                .add("size", static_cast<double>(bin.size))
                .add("last_modified", static_cast<double>(bin.last_modified))
                .add("type", bin.type)
                .build()
        );
    }
    return arr.to_string();
}

#include "MISC/elf_management/shared_library.hpp"

std::string handle_api_libraries(const std::string& workspace_path) {
    auto libs = utils::elf_management::scan_libraries(workspace_path);
    dj::Array arr;
    for (const auto& lib : libs) {
        arr.push(
            dj::Builder()
                .add("name", lib.name)
                .add("path", lib.path)
                .add("target_name", lib.target_name)
                .add("file_size", static_cast<double>(lib.file_size))
                .add("last_modified", static_cast<double>(lib.last_modified))
                .add("make_command", lib.make_command)
                .add("lib_name", lib.lib_name)
                .add("version", lib.version)
                .add("description", lib.description)
                .add("author", lib.author)
                .add("has_metadata", lib.has_metadata)
                .build()
        );
    }
    return arr.to_string();
}

std::string handle_api_libdocs(const std::string& libdocs_path) {
    dj::Array arr;
    std::string cmd = "find " + libdocs_path + " -type f -name index.html 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string path(buffer);
            if (!path.empty() && path.back() == '\n') path.pop_back();
            arr.push(path);
        }
        pclose(pipe);
    }
    return arr.to_string();
}

dj::Array scan_simple_dir(const std::string& dir, const std::string& ext = ".cpp") {
    dj::Array arr;
    DIR* d = opendir(dir.c_str());
    if (!d) return arr;
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (entry->d_type == DT_REG && (ext.empty() || name.size() > ext.size() && name.substr(name.size()-ext.size()) == ext)) {
            arr.push(name);
        }
    }
    closedir(d);
    return arr;
}

std::string handle_api_demos() {
    auto arr = scan_simple_dir("_binaries/demos", ".cpp");
    return arr.to_string();
}

std::string handle_api_services() {
    auto arr = scan_simple_dir("_binaries/services", ".cpp");
    return arr.to_string();
}

std::string handle_api_apps() {
    auto arr = scan_simple_dir("_binaries/apps", ".cpp");
    return arr.to_string();
}

std::string handle_api_ui() {
    std::ifstream file("config/resources/html/service_manager.html");
    if (!file.is_open()) {
        return "<html><body><h1>Service Manager UI</h1><p>Could not open service_manager.html</p></body></html>";
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string handle_api_rebuild(const std::string& workspace_path, const std::string& target) {
    return "{\"result\":\"rebuild started\"}";
}

std::string handle_api_docs(const std::string& request) {
    return "<html><body><h1>Documentation</h1></body></html>";
}

std::string handle_api_docs_rebuild() {
    return "{\"result\":\"docs rebuild started\"}";
}

} // namespace service_manager
