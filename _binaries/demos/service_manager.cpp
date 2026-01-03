#include "service_manager/request_handlers.h"
#include "service_manager/server_constants.hpp"
#include "dataformats/http/response_dataframe.h"
/*
 * Service Manager - Shared Library Build Management
 * 
 * A focused tool for managing and rebuilding shared C++ libraries.
 * 
 * Features:
 * - Displays all .so/.dylib files in build/libraries/src/
 * - Shows library details (size, last modified, make target)
 * - Individual rebuild buttons for each library
 * - Real-time library scanning
 * 
 * Usage: ./service_manager [port]
 * Default port: 9004
 * 
 * Access at: http://localhost:9004
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <memory>
#include "networking/rest_api/http3_server.h"
#include "dataformats/json/json.h"
#include "dataformats/http/http.h"

// ...existing code...
#include "service_manager/binary_info.hpp"

std::vector<BinaryInfo> scan_binaries() {
    std::vector<BinaryInfo> bins;
    std::string bin_dir = "build/bin";
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
/*
 * Service Manager - Shared Library Build Management
 * 
 * A focused tool for managing and rebuilding shared C++ libraries.
 * 
 * Features:
 * - Displays all .so/.dylib files in build/libraries/src/
 * - Shows library details (size, last modified, make target)
 * - Individual rebuild buttons for each library
 * - Real-time library scanning
 * 
 * Usage: ./service_manager [port]
 * Default port: 9004
 * 
 * Access at: http://localhost:9004
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <dlfcn.h>

// ...existing code...
#include "service_manager/shared_library.hpp"

std::vector<SharedLibrary> scan_libraries() {
    std::vector<SharedLibrary> libraries;
    
    std::string cmd = "find /Users/mehranghamaty/wkspace/ToolBox/build/src -type f \\( -name '*.so' -o -name '*.dylib' \\) 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return libraries;
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string lib_path(buffer);
        lib_path.erase(lib_path.find_last_not_of(" \n\r\t") + 1);
        
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
        
        // Try to extract embedded metadata
        void* handle = dlopen(lib_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (handle) {
            typedef const char* (*get_string_func)();
            
            auto get_name = (get_string_func)dlsym(handle, "get_library_name");
            auto get_version = (get_string_func)dlsym(handle, "get_library_version");
            auto get_desc = (get_string_func)dlsym(handle, "get_library_description");
            auto get_author = (get_string_func)dlsym(handle, "get_library_author");
            auto get_doc = (get_string_func)dlsym(handle, "get_library_doc");
            
            if (get_name && get_version && get_desc && get_author) {
                lib.lib_name = get_name();
                lib.version = get_version();
                lib.description = get_desc();
                lib.author = get_author();
                lib.has_metadata = true;
            } else if (get_doc) {
                lib.description = get_doc();
                lib.has_metadata = true;
            }
            
            dlclose(handle);
        }
        
        libraries.push_back(lib);
    }
    pclose(pipe);
    
    return libraries;
}

bool rebuild_library(const std::string& target) {
    std::string cmd = "cd /Users/mehranghamaty/wkspace/ToolBox && cmake --build build --target " + target + " -j8 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    
    char buffer[256];
    bool success = true;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        if (line.find("error:") != std::string::npos) success = false;
    }
    
    int exit_code = pclose(pipe);
    return (exit_code == 0 && success);
}



void handle_request(int client_fd) {

    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    std::string request(buffer);
    std::string response;

    if (request.find("POST /api/refresh-binaries") == 0) {
        std::cout << "[HTTP REQUEST] POST /api/refresh-binaries\n";
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":true}";
        write(client_fd, response.c_str(), response.length());
        close(client_fd);
        return;
    }
    if (request.find("POST /api/refresh-libraries") == 0) {
        std::cout << "[HTTP REQUEST] POST /api/refresh-libraries\n";
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":true}";
        write(client_fd, response.c_str(), response.length());
        close(client_fd);
        return;
    }

    if (request.find("POST /api/rebuild-all") == 0) {
        // Rebuild all shared libraries and binaries
        int ret = system("cmake --build build -j8");
        response = std::string("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":") + (ret==0?"true":"false") + "}";
        write(client_fd, response.c_str(), response.length());
        close(client_fd);
        return;
    }

    // Log the incoming HTTP request line (first line)
    size_t req_end = request.find("\r\n");
    if (req_end != std::string::npos) {
        std::string req_line = request.substr(0, req_end);
        std::cout << "[HTTP REQUEST] " << req_line << std::endl;
    } else {
        std::cout << "[HTTP REQUEST] " << request << std::endl;
    }
    
    if (request.find("GET /api/routes") == 0) {
        // List all exposed API routes using shared Route object and ResponseDataFrame
        #include "service_manager/server_constants.hpp"
        #include "dataformats/http/response_dataframe.h"
        const auto& route_objs = service_manager::get_api_routes();
        std::vector<std::string> columns = {"method", "path", "description"};
        dataformats::http::ResponseDataFrame df(columns);
        for (const auto& r : route_objs) {
            df.add_row({r.method, r.path, r.description});
        }
        df.set_metadata("count", std::to_string(route_objs.size()));
        dataformats::http::Response http_resp;
        http_resp.status_code = 200;
        http_resp.headers["Content-Type"] = "application/json";
        http_resp.json_body = df.to_json();
        http_resp.body = http_resp.json_body.to_string();
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + http_resp.body;
    } else if (request.find("GET /api/binaries") == 0) {
        auto bins = scan_binaries();
        std::vector<std::string> columns = {"name", "path", "size", "last_modified", "type"};
        dataformats::http::ResponseDataFrame df(columns);
        for (const auto& bin : bins) {
            df.add_row({bin.name, bin.path, static_cast<double>(bin.size), static_cast<double>(bin.last_modified), bin.type});
        }
        df.set_metadata("count", std::to_string(bins.size()));
        std::string json_str = df.to_json().to_string();
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json_str;
    } else if (request.find("GET /api/libdocs") == 0) {
        // Recursively find all per-library docs index.html files
        std::vector<std::string> doc_paths;
        std::string find_cmd = "find gen_docs/html/libs -type f -name index.html 2>/dev/null";
        FILE* pipe = popen(find_cmd.c_str(), "r");
        if (pipe) {
            char buf[1024];
            while (fgets(buf, sizeof(buf), pipe)) {
                std::string p(buf);
                p.erase(p.find_last_not_of(" \n\r\t") + 1);
                // Convert to /docs/libs/.../html/index.html for browser
                if (p.find("gen_docs/html/") == 0) {
                    p = "/docs/" + p.substr(14); // 14 = strlen("gen_docs/html/")
                }
                doc_paths.push_back(p);
            }
            pclose(pipe);
        }
        std::ostringstream json;
        json << "{\"libs\":[";
        for (size_t i = 0; i < doc_paths.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << doc_paths[i] << "\"";
        }
        json << "]}";
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json.str();
    } else if (request.find("GET / ") == 0 || request.find("GET /index") == 0) {
        // Serve the main HTML UI from the resources directory
            std::ifstream htmlFile("config/resources/html/service_manager.html");
        if (htmlFile) {
            std::string html((std::istreambuf_iterator<char>(htmlFile)), std::istreambuf_iterator<char>());
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
        } else {
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFailed to load UI.";
        }
    } else if (request.find("GET /api/libraries") == 0) {
        auto libs = scan_libraries();
        std::ostringstream json;
        json << "{\"libraries\":[";
        for (size_t i = 0; i < libs.size(); i++) {
            if (i > 0) json << ",";
            json << "{"
                 << "\"name\":\"" << libs[i].name << "\","
                 << "\"path\":\"" << libs[i].path << "\","
                 << "\"target\":\"" << libs[i].target_name << "\","
                 << "\"size\":" << libs[i].file_size << ","
                 << "\"last_modified\":" << libs[i].last_modified << ","
                 << "\"make_command\":\"" << libs[i].make_command << "\","
                 << "\"has_metadata\":" << (libs[i].has_metadata ? "true" : "false");
            if (libs[i].has_metadata) {
                json << ",\"lib_name\":\"" << libs[i].lib_name << "\""
                     << ",\"version\":\"" << libs[i].version << "\""
                     << ",\"description\":\"" << libs[i].description << "\""
                     << ",\"author\":\"" << libs[i].author << "\"";
            }
            json << "}";
        }
        json << "]}";
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json.str();
    } else if (request.find("POST /api/rebuild") == 0) {
        size_t body_pos = request.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            std::string body = request.substr(body_pos + 4);
            size_t target_pos = body.find("\"target\":\"");
            if (target_pos != std::string::npos) {
                size_t start = target_pos + 10;
                size_t end = body.find("\"", start);
                std::string target = body.substr(start, end - start);
                bool success = rebuild_library(target);
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":" + 
                          std::string(success ? "true" : "false") + "}";
            }
        }
    } else if (request.find("GET /docs/") == 0) {
        std::string path = request.substr(request.find("/docs/"));
        std::string file_path1 = "gen_docs/html/" + path.substr(6); // strip /docs/
        std::string file_path2 = "gen_docs/html/html/" + path.substr(6); // fallback: html/html/
        FILE* f = fopen(file_path1.c_str(), "rb");
        if (!f) {
            f = fopen(file_path2.c_str(), "rb");
        }
        if (f) {
            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            std::vector<char> buf(size);
            fread(buf.data(), 1, size, f);
            fclose(f);
            std::string mime = (file_path1.find(".html")!=std::string::npos||file_path2.find(".html")!=std::string::npos)?"text/html":"application/octet-stream";
            response = "HTTP/1.1 200 OK\r\nContent-Type: "+mime+"\r\n\r\n" + std::string(buf.begin(), buf.end());
        } else {
            response = "HTTP/1.1 404 Not Found\r\n\r\nDoc Not Found";
        }
    } else if (request.find("POST /api/docs-rebuild") == 0) {
        int ret = system("make docs-rebuild");
        response = std::string("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{") +
            "\"success\":" + (ret==0?"true":"false") + "}";
    } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
    }
    
    write(client_fd, response.c_str(), response.length());
    close(client_fd);
}


int main(int argc, char* argv[]) {
    int port = 9004;
    size_t num_threads = 8;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port < 1024 || port > 65535) port = 9004;
    }

#include "service_manager/server_constants.hpp"

    const auto& routes = service_manager::get_api_routes();

    std::cout << "Exposed API Routes:\n";
    for (const auto& r : routes) {
        std::cout << "  [" << r.method << "] " << r.path << "\n      - " << r.description << "\n";
    }

    auto server = std::make_unique<ml::rest_api::Http3Server>(port, num_threads);
    server->enable_cors();
    server->enable_0rtt(true);
    server->set_max_idle_timeout(60000);

    server->load_routes(routes);

    server->start();
    while (server->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
