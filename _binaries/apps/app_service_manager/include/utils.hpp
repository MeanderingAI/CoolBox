#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include "request_handlers.h"
#include "IO/dataformats/json/json.h"
#include "IO/dataformats/http/request_response.h"

namespace dj = dataformats::json;

// Main HTTP request handler declaration (should match your route handler signatures)
Response handle_request(const Request& req);

class Commands {
public:
    static std::string find_libdocs_command(const std::string& libdocs_path) {
        return "find " + libdocs_path + " -type f -name index.html 2>/dev/null";
    }
    static std::string docs_rebuild_command() {
        return "make docs-rebuild";
    }
};

// API handler function declarations
std::string handle_api_routes();
std::string handle_api_binaries(const std::string& workspace_path);
std::string handle_api_libraries(const std::string& workspace_path);
std::string handle_api_libdocs(const std::string& libdocs_path);
std::string handle_api_demos();
std::string handle_api_services();
std::string handle_api_apps();
std::string handle_api_ui();
std::string handle_api_rebuild(const std::string& workspace_path, const std::string& target);
std::string handle_api_docs(const std::string& request);
std::string handle_api_docs_rebuild();

class handler_settings {
public:
    std::string workspace_path;
    std::string libdocs_path;
    handler_settings(const std::string& path, const std::string& libdocs) : workspace_path(path), libdocs_path(libdocs) {}
    static handler_settings from_config(const std::string& config_path) {
        std::string workspace_path = ".";
        std::string libdocs_path = "gen_docs/html/libs";
        std::ifstream config_file(config_path);
        if (config_file) {
            std::string content((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
            auto config_json = dj::Parser::parse(content);
            if (config_json.is_object()) {
                auto obj = config_json.as_object();
                if (obj.has("workspace_path")) workspace_path = obj.get("workspace_path").as_string();
                if (obj.has("libdocs_path")) libdocs_path = obj.get("libdocs_path").as_string();
            }
        }
        return handler_settings(workspace_path, libdocs_path);
    }
};
