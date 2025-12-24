#include "networking/html/web_components.h"
#include "auth/auth_system.h"
#include "services/cache_server/cache_server.h"
#include "services/distributed_fs/distributed_fs.h"
#include "services/mail_server/mail_server.h"
#include "services/url_shortener/url_shortener.h"
#include "services/system_monitor/system_monitor.h"
#include "services/service_breaker/service_breaker.h"
#include "ml_server/ml_server.h"
#include "app_launcher_html.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <map>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <vector>
#include <atomic>

using namespace ml::networking::html;
using namespace auth;

class MATLABStyleUI {
public:
    MATLABStyleUI(int port, AuthSystem* auth_system, bool enable_hot_reload = true) 
        : port_(port), auth_system_(auth_system), running_(false), url_shortener_(), 
          enable_hot_reload_(enable_hot_reload) {
        std::cout << "🔥 Hot-reload: " << (enable_hot_reload ? "ENABLED" : "DISABLED") << "\n";
    }
    
    void start() {
        running_ = true;
        
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        bind(server_fd_, (struct sockaddr*)&address, sizeof(address));
        listen(server_fd_, 10);
        
        std::cout << "✓ MATLAB-Style App Launcher running on http://localhost:" << port_ << "\n";
        
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) continue;
            
            handle_request(client_fd);
            close(client_fd);
        }
    }
    
private:
    int port_;
    int server_fd_;
    bool running_;
    AuthSystem* auth_system_;
    std::map<std::string, std::string> session_cache_;
    services::URLShortener url_shortener_;
    services::SystemMonitor system_monitor_;
    services::ServiceBreaker service_breaker_;
    bool enable_hot_reload_;
    std::map<std::string, std::pair<std::string, time_t>> html_cache_; // path -> (content, mtime)
    
    void handle_request(int client_fd) {
        char buffer[16384] = {0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) return;
        
        std::string request(buffer, bytes_read);
        
        // Check if we need to read more for POST body
        size_t content_length = 0;
        size_t cl_pos = request.find("Content-Length:");
        if (cl_pos != std::string::npos) {
            size_t cl_start = cl_pos + 15;
            size_t cl_end = request.find("\r\n", cl_start);
            std::string cl_str = request.substr(cl_start, cl_end - cl_start);
            // Trim whitespace
            size_t first = cl_str.find_first_not_of(" \t");
            size_t last = cl_str.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos) {
                cl_str = cl_str.substr(first, last - first + 1);
                content_length = std::stoul(cl_str);
            }
        }
        
        // Check if we have the full body
        size_t header_end = request.find("\r\n\r\n");
        if (header_end != std::string::npos && content_length > 0) {
            size_t body_start = header_end + 4;
            size_t body_received = bytes_read - body_start;
            
            // Read more if needed
            while (body_received < content_length && bytes_read < (ssize_t)sizeof(buffer) - 1) {
                ssize_t more = read(client_fd, buffer + bytes_read, sizeof(buffer) - bytes_read - 1);
                if (more <= 0) break;
                bytes_read += more;
                body_received += more;
            }
            request = std::string(buffer, bytes_read);
        }
        
        // Parse request
        size_t path_start = request.find(" ") + 1;
        size_t path_end = request.find(" ", path_start);
        std::string path = request.substr(path_start, path_end - path_start);
        
        size_t query_pos = path.find('?');
        std::string query;
        if (query_pos != std::string::npos) {
            query = path.substr(query_pos + 1);
            path = path.substr(0, query_pos);
        }
        
        std::string response;
        
        // Route handling - Passwordless mode
        if (path == "/" || path == "/login") {
            response = generate_app_launcher("");
        } else if (path == "/app/cache") {
            response = generate_cache_app("");
        } else if (path == "/app/dfs") {
            response = generate_dfs_app("");
        } else if (path == "/app/mail") {
            response = generate_mail_app("");
        } else if (path == "/app/ml") {
            response = generate_ml_app("");
        } else if (path == "/app/security") {
            response = generate_security_app("");
        } else if (path == "/app/dns") {
            response = generate_dns_app("");
        } else if (path == "/app/proxy") {
            response = generate_proxy_app("");
        } else if (path == "/app/urlshort") {
            response = generate_url_shortener_app("");
        } else if (path == "/app/monitor") {
            response = generate_system_monitor_app("");
        } else if (path == "/app/breaker") {
            response = generate_service_breaker_app("");
        } else if (path == "/admin") {
            response = generate_admin_panel("");
        } else if (path == "/account") {
            response = generate_account_management("");
        } else if (path == "/api/create_user" && request.find("POST") != std::string::npos) {
            // Handle user creation
            std::string post_data = extract_post_data(request);
            std::map<std::string, std::string> params;
            std::istringstream ss(post_data);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    std::string value = url_decode(pair.substr(eq + 1));
                    params[key] = value;
                }
            }
            
            if (!params["username"].empty() && !params["password"].empty()) {
                UserRole role = params["role"] == "admin" ? UserRole::ADMIN : UserRole::USER;
                bool success = auth_system_->create_user(params["username"], params["password"], 
                                                         params["email"], role);
                std::string message = success ? "User created successfully" : "Failed to create user";
                response = "HTTP/1.1 302 Found\r\nLocation: /admin?msg=" + message + "\r\n\r\n";
                write(client_fd, response.c_str(), response.length());
                return;
            }
            response = generate_admin_panel("");
        } else if (path == "/api/start_account_server") {
            // Simulate starting account server
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\",\"message\":\"Account server started on port 8888\"}";
            write(client_fd, response.c_str(), response.length());
            std::cout << "\n✓ Account server started on port 8888\n";
            return;
        } else if (path == "/api/shorten" && request.find("POST") != std::string::npos) {
            // Handle URL shortening
            std::string post_data = extract_post_data(request);
            std::map<std::string, std::string> params;
            std::istringstream ss(post_data);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    std::string value = url_decode(pair.substr(eq + 1));
                    params[key] = value;
                }
            }
            
            std::string short_code = url_shortener_.shorten_url(params["url"], params["custom"]);
            if (!short_code.empty()) {
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\",\"short_code\":\"" + short_code + "\",\"short_url\":\"http://localhost:9000/s/" + short_code + "\"}";
            } else {
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\",\"message\":\"Invalid URL or custom code already taken\"}";
            }
            write(client_fd, response.c_str(), response.length());
            return;
        } else if (path.substr(0, 3) == "/s/") {
            // Redirect short URL
            std::string short_code = path.substr(3);
            std::string original_url = url_shortener_.resolve_url(short_code);
            if (!original_url.empty()) {
                response = "HTTP/1.1 302 Found\r\nLocation: " + original_url + "\r\n\r\n";
            } else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 - Short URL not found</h1>";
            }
            write(client_fd, response.c_str(), response.length());
            return;
        } else if (path == "/api/service/start" && request.find("POST") != std::string::npos) {
            // Handle service start
            std::string post_data = extract_post_data(request);
            std::map<std::string, std::string> params;
            std::istringstream ss(post_data);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    std::string value = url_decode(pair.substr(eq + 1));
                    params[key] = value;
                }
            }
            
            bool success = service_breaker_.start_service(params["service"]);
            response = success ? 
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\"}" :
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\"}";
            write(client_fd, response.c_str(), response.length());
            return;
        } else if (path == "/api/service/stop" && request.find("POST") != std::string::npos) {
            // Handle service stop
            std::string post_data = extract_post_data(request);
            std::map<std::string, std::string> params;
            std::istringstream ss(post_data);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    std::string value = url_decode(pair.substr(eq + 1));
                    params[key] = value;
                }
            }
            
            bool success = service_breaker_.stop_service(params["service"]);
            response = success ? 
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\"}" :
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\"}";
            write(client_fd, response.c_str(), response.length());
            return;
        } else if (path == "/api/service/setport" && request.find("POST") != std::string::npos) {
            // Handle service port change
            std::string post_data = extract_post_data(request);
            std::map<std::string, std::string> params;
            std::istringstream ss(post_data);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    std::string value = url_decode(pair.substr(eq + 1));
                    params[key] = value;
                }
            }
            
            int port = std::stoi(params["port"]);
            bool success = service_breaker_.set_port(params["service"], port);
            response = success ? 
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\"}" :
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\"}";
            write(client_fd, response.c_str(), response.length());
            return;
        } else if (path == "/api/metrics") {
            // Return current system metrics as JSON
            system_monitor_.update();
            auto metrics = system_monitor_.get_metrics();
            
            std::stringstream json;
            json << std::fixed << std::setprecision(2);
            json << "{"
                 << "\"cpu_usage\":" << metrics.cpu_usage << ","
                 << "\"memory_usage\":" << metrics.memory_usage << ","
                 << "\"memory_used_mb\":" << metrics.memory_used_mb << ","
                 << "\"memory_total_mb\":" << metrics.memory_total_mb << ","
                 << "\"disk_usage\":" << metrics.disk_usage << ","
                 << "\"disk_used_gb\":" << metrics.disk_used_gb << ","
                 << "\"disk_total_gb\":" << metrics.disk_total_gb << ","
                 << "\"network_rx_mbps\":" << metrics.network_rx_mbps << ","
                 << "\"network_tx_mbps\":" << metrics.network_tx_mbps << ","
                 << "\"network_rx_bytes\":" << metrics.network_rx_bytes << ","
                 << "\"network_tx_bytes\":" << metrics.network_tx_bytes << ","
                 << "\"process_count\":" << metrics.process_count << ","
                 << "\"uptime\":\"" << metrics.uptime << "\","
                 << "\"timestamp\":\"" << metrics.timestamp << "\""
                 << "}";
            
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: no-cache\r\n\r\n" + json.str();
            write(client_fd, response.c_str(), response.length());
            return;
        } else {
            response = generate_app_launcher("");
        }
        
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response;
        
        write(client_fd, http_response.c_str(), http_response.length());
    }
    
    std::string extract_session_cookie(const std::string& request) {
        size_t cookie_pos = request.find("Cookie:");
        if (cookie_pos == std::string::npos) return "";
        
        size_t start = request.find("session_id=", cookie_pos);
        if (start == std::string::npos) return "";
        
        start += 11;  // length of "session_id="
        size_t end = request.find_first_of(";\r\n", start);
        if (end == std::string::npos) end = request.length();
        
        return request.substr(start, end - start);
    }
    
    std::string extract_post_data(const std::string& request) {
        std::cout << "\n[extract_post_data] Request length: " << request.length() << "\n";
        std::cout << "[extract_post_data] First 500 chars: " << request.substr(0, 500) << "\n";
        
        size_t pos = request.find("\r\n\r\n");
        if (pos == std::string::npos) {
            std::cout << "[extract_post_data] No \\r\\n\\r\\n found!\n";
            return "";
        }
        
        std::string body = request.substr(pos + 4);
        std::cout << "[extract_post_data] Body extracted: [" << body << "]\n";
        std::cout << "[extract_post_data] Body length: " << body.length() << "\n";
        return body;
    }
    
    std::string url_decode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); i++) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int value;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    AuthResult handle_login(const std::string& post_data) {
        std::map<std::string, std::string> params;
        std::istringstream ss(post_data);
        std::string pair;
        
        std::cout << "Parsing form data...\n";
        while (std::getline(ss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string key = pair.substr(0, eq);
                std::string value = url_decode(pair.substr(eq + 1));
                params[key] = value;
                std::cout << "  " << key << " = [" << value << "] (length: " << value.length() << ")\n";
            }
        }
        
        std::cout << "\nAttempting login...\n";
        std::cout << "  Username: '" << params["username"] << "'\n";
        std::cout << "  Password: '" << params["password"] << "'\n";
        std::cout << "  Password length: " << params["password"].length() << "\n";
        
        auto result = auth_system_->login(params["username"], params["password"]);
        
        if (result.success) {
            std::cout << "✓ Login successful for user: " << params["username"] << "\n";
            std::cout << "  Session ID: " << result.session_id << "\n";
        } else {
            std::cout << "✗ Login failed: " << result.message << "\n";
        }
        
        return result;
    }
    
    std::string load_html_with_reload(const std::string& file_path) {
        if (!enable_hot_reload_) {
            // Hot-reload disabled, check cache only
            auto it = html_cache_.find(file_path);
            if (it != html_cache_.end()) {
                return it->second.first;
            }
            return "";
        }
        
        // Check if file exists and get modification time
        struct stat file_stat;
        if (stat(file_path.c_str(), &file_stat) != 0) {
            // File doesn't exist, return cached or empty
            auto it = html_cache_.find(file_path);
            return (it != html_cache_.end()) ? it->second.first : "";
        }
        
        time_t mtime = file_stat.st_mtime;
        
        // Check if we have cached and if it's up to date
        auto it = html_cache_.find(file_path);
        if (it != html_cache_.end() && it->second.second >= mtime) {
            return it->second.first; // Cache is fresh
        }
        
        // Read file
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cout << "⚠️  Failed to open: " << file_path << "\\n";
            return (it != html_cache_.end()) ? it->second.first : "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        // Update cache
        html_cache_[file_path] = {content, mtime};
        std::cout << "🔄 Hot-reloaded: " << file_path << "\\n";
        
        return content;
    }
    
    std::string generate_login_page(const std::string& error = "") {
        ComponentBundler bundler;
        
        std::string error_html;
        if (!error.empty()) {
            error_html = "<div style=\"background: #fee; border: 1px solid #fcc; padding: 1rem; border-radius: 4px; margin-bottom: 1rem; color: #c33;\">" + error + "</div>";
        }
        
        return bundler
            .set_title("Login - MATLAB Style Platform")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body {
                    font-family: Arial, Helvetica, sans-serif;
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    min-height: 100vh;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                }
                .login-container {
                    background: white;
                    padding: 3rem;
                    border-radius: 12px;
                    box-shadow: 0 20px 60px rgba(0,0,0,0.3);
                    width: 400px;
                    max-width: 90%;
                }
                .logo {
                    text-align: center;
                    font-size: 2.5rem;
                    color: #667eea;
                    margin-bottom: 2rem;
                    font-weight: bold;
                }
                .login-form input {
                    width: 100%;
                    padding: 0.75rem;
                    margin-bottom: 1rem;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 1rem;
                }
                .login-form button {
                    width: 100%;
                    padding: 0.75rem;
                    background: #667eea;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    font-size: 1rem;
                    cursor: pointer;
                    font-weight: bold;
                }
                .login-form button:hover {
                    background: #5568d3;
                }
                .hint {
                    text-align: center;
                    color: #666;
                    font-size: 0.9rem;
                    margin-top: 1rem;
                }
            )")
            .set_body_content(R"(
                <div class="login-container">
                    <div class="logo">🔬 ToolBox Platform</div>
                    )" + error_html + R"(
                    <form class="login-form" method="POST" action="/api/login">
                        <input type="text" name="username" placeholder="Username" required>
                        <input type="password" name="password" placeholder="Password" required>
                        <button type="submit">Sign In</button>
                    </form>
                    <div class="hint">
                        Default: admin/admin123 or user/user123
                    </div>
                </div>
            )")
            .minify(true)
            .bundle();
    }
    
    std::string generate_app_launcher(const std::string& session_id) {
        std::string username = "Guest";
        std::string role = "User";
        
        // Try hot-reload from source file first
        std::string html_path = "../demos/resources/html/app_launcher.html";
        std::string html = load_html_with_reload(html_path);
        
        // Fall back to embedded resource if hot-reload fails
        if (html.empty()) {
            html = resources::APP_LAUNCHER_HTML;
        }
        
        // Replace placeholders
        size_t pos = html.find("{{USERNAME}}");
        if (pos != std::string::npos) {
            html.replace(pos, 12, username);
        }
        pos = html.find("{{ROLE}}");
        if (pos != std::string::npos) {
            html.replace(pos, 8, role);
        }
        
        return html;
    }
    
    std::string generate_cache_app(const std::string& session_id) {
        std::string content = 
                "<h3>Cache Operations</h3>"
                "<form-input label=\"Key\" placeholder=\"my_key\"></form-input>"
                "<form-input label=\"Value\" placeholder=\"my_value\"></form-input>"
                "<form-input label=\"TTL seconds\" placeholder=\"3600\"></form-input>"
                "<app-button>Set Cache</app-button>"
                "<app-button>Get Cache</app-button>"
                "<app-button>Delete Cache</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Statistics</h3>"
                "<progress-bar value=\"75\" max=\"100\"></progress-bar>"
                "<p>Cache Hit Rate: 75%</p>"
                "<progress-bar value=\"4096\" max=\"10000\"></progress-bar>"
                "<p>Memory Usage: 4096/10000 MB</p>";
        return generate_generic_app(session_id, "Cache Server", "💾", 
            "Manage high-performance distributed caching", content);
    }
    
    std::string generate_dfs_app(const std::string& session_id) {
        std::string content =
                "<h3>File Operations</h3>"
                "<form-input label=\"Filename\" placeholder=\"/path/to/file.txt\"></form-input>"
                "<form-input label=\"Content\" placeholder=\"File content...\"></form-input>"
                "<form-input label=\"Replication Factor\" placeholder=\"3\"></form-input>"
                "<app-button>Upload File</app-button>"
                "<app-button>Download File</app-button>"
                "<app-button>Delete File</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Storage Nodes</h3>"
                "<data-table>"
                "    <tr slot=\"header\"><th>Node</th><th>Status</th><th>Storage</th></tr>"
                "    <tr><td>node-1</td><td>✓ Online</td><td>500 GB</td></tr>"
                "    <tr><td>node-2</td><td>✓ Online</td><td>500 GB</td></tr>"
                "    <tr><td>node-3</td><td>✓ Online</td><td>500 GB</td></tr>"
                "</data-table>";
        return generate_generic_app(session_id, "Distributed File System", "📁",
            "Manage distributed file storage and replication", content);
    }
    
    std::string generate_mail_app(const std::string& session_id) {
        std::string content =
                "<h3>Compose Email</h3>"
                "<form-input label=\"To\" placeholder=\"user@example.com\"></form-input>"
                "<form-input label=\"Subject\" placeholder=\"Subject\"></form-input>"
                "<form-input label=\"Message\" placeholder=\"Email body...\"></form-input>"
                "<app-button>Send Email</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Inbox</h3>"
                "<data-table>"
                "    <tr slot=\"header\"><th>From</th><th>Subject</th><th>Date</th></tr>"
                "    <tr><td>admin@localhost</td><td>Welcome</td><td>Today</td></tr>"
                "    <tr><td>system@localhost</td><td>Alert</td><td>Yesterday</td></tr>"
                "</data-table>";
        return generate_generic_app(session_id, "Mail Server", "📧",
            "SMTP/POP3 email server management", content);
    }
    
    std::string generate_ml_app(const std::string& session_id) {
        std::string content =
                "<h3>Model Selection</h3>"
                "<form-input label=\"Model\" placeholder=\"Select model...\"></form-input>"
                "<form-input label=\"Dataset\" placeholder=\"Select dataset...\"></form-input>"
                "<h3>Input Features</h3>"
                "<form-input label=\"Feature 1\" placeholder=\"0.5\"></form-input>"
                "<form-input label=\"Feature 2\" placeholder=\"1.2\"></form-input>"
                "<form-input label=\"Feature 3\" placeholder=\"-0.3\"></form-input>"
                "<app-button>Run Prediction</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Model Performance</h3>"
                "<progress-bar value=\"92\" max=\"100\"></progress-bar>"
                "<p>Accuracy: 92%</p>";
        return generate_generic_app(session_id, "ML Model Server", "🤖",
            "Machine learning model deployment and serving", content);
    }
    
    std::string generate_security_app(const std::string& session_id) {
        std::string content =
                "<h3>Scan Operations</h3>"
                "<form-input label=\"Content\" placeholder=\"Paste content to scan...\"></form-input>"
                "<app-button>Scan Content</app-button>"
                "<app-button>Scan File</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Threat Statistics</h3>"
                "<progress-bar value=\"95\" max=\"100\"></progress-bar>"
                "<p>System Health: 95%</p>"
                "<data-table>"
                "    <tr slot=\"header\"><th>Type</th><th>Count</th><th>Status</th></tr>"
                "    <tr><td>Malware</td><td>0</td><td>✓ Clean</td></tr>"
                "    <tr><td>Suspicious</td><td>2</td><td>⚠ Review</td></tr>"
                "</data-table>";
        return generate_generic_app(session_id, "Security Scanner", "🔒",
            "Malware detection and content security", content);
    }
    
    std::string generate_dns_app(const std::string& session_id) {
        std::string content =
                "<h3>DNS Lookup</h3>"
                "<form-input label=\"Domain\" placeholder=\"example.com\"></form-input>"
                "<app-button>Lookup</app-button>"
                "<h3 style=\"margin-top: 2rem;\">DNS Records</h3>"
                "<data-table>"
                "    <tr slot=\"header\"><th>Domain</th><th>Type</th><th>Value</th></tr>"
                "    <tr><td>localhost</td><td>A</td><td>127.0.0.1</td></tr>"
                "    <tr><td>api.local</td><td>A</td><td>192.168.1.100</td></tr>"
                "</data-table>";
        return generate_generic_app(session_id, "DNS Server", "🌐",
            "Domain name resolution service", content);
    }
    
    std::string generate_proxy_app(const std::string& session_id) {
        std::string content =
                "<h3>Proxy Configuration</h3>"
                "<form-input label=\"Target URL\" placeholder=\"http://backend:8080\"></form-input>"
                "<form-input label=\"Port\" placeholder=\"8081\"></form-input>"
                "<app-button>Start Proxy</app-button>"
                "<app-button>Stop Proxy</app-button>"
                "<h3 style=\"margin-top: 2rem;\">Traffic Statistics</h3>"
                "<progress-bar value=\"45\" max=\"100\"></progress-bar>"
                "<p>CPU Usage: 45%</p>"
                "<p>Requests/sec: 1250</p>";
        return generate_generic_app(session_id, "Proxy Server", "🔄",
            "HTTP/HTTPS proxy and load balancing", content);
    }
    
    std::string generate_url_shortener_app(const std::string& session_id) {
        auto all_urls = url_shortener_.get_all_urls();
        std::stringstream urls_table;
        
        for (const auto& [code, url_info] : all_urls) {
            urls_table << "<tr>"
                      << "<td><a href='/s/" << code << "' target='_blank'>" << code << "</a></td>"
                      << "<td style='max-width: 300px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;'>" 
                      << url_info.original_url << "</td>"
                      << "<td>" << url_info.click_count << "</td>"
                      << "<td>" << url_info.created_at << "</td>"
                      << "</tr>";
        }
        
        std::string content =
                "<h3>Shorten URL</h3>"
                "<form id='shortenForm' onsubmit='return shortenURL(event);' style='background: #f8f9fa; padding: 1.5rem; border-radius: 8px; margin-bottom: 2rem;'>"
                "    <div style='margin-bottom: 1rem;'>"
                "        <label style='display: block; margin-bottom: 0.5rem; font-weight: bold;'>Long URL</label>"
                "        <input type='url' id='longUrl' required placeholder='https://example.com/very/long/url' "
                "               style='width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;'>"
                "    </div>"
                "    <div style='margin-bottom: 1rem;'>"
                "        <label style='display: block; margin-bottom: 0.5rem; font-weight: bold;'>Custom Code (optional)</label>"
                "        <input type='text' id='customCode' placeholder='mylink' "
                "               style='width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;'>"
                "    </div>"
                "    <button type='submit' style='background: #3498db; color: white; border: none; padding: 0.75rem 2rem; border-radius: 4px; cursor: pointer; font-size: 1rem;'>"
                "        🔗 Shorten URL"
                "    </button>"
                "</form>"
                "<div id='result' style='display: none; background: #d4edda; border: 1px solid #c3e6cb; padding: 1rem; border-radius: 4px; margin-bottom: 2rem;'>"
                "    <strong>✓ URL Shortened!</strong><br>"
                "    <p style='margin-top: 0.5rem;'>Short URL: <a id='shortUrl' href='#' target='_blank' style='color: #155724; font-weight: bold;'></a></p>"
                "    <button onclick='copyToClipboard()' style='background: #28a745; color: white; border: none; padding: 0.5rem 1rem; border-radius: 4px; cursor: pointer; margin-top: 0.5rem;'>"
                "        📋 Copy Link"
                "    </button>"
                "</div>"
                "<h3>Statistics</h3>"
                "<div style='background: white; padding: 1rem; border-radius: 8px; margin-bottom: 2rem;'>"
                "    <p><strong>Total URLs:</strong> " + std::to_string(url_shortener_.get_total_urls()) + "</p>"
                "    <p><strong>Total Clicks:</strong> " + std::to_string(url_shortener_.get_total_clicks()) + "</p>"
                "</div>"
                "<h3>Recent URLs</h3>"
                "<div style='background: white; padding: 1.5rem; border-radius: 8px; overflow-x: auto;'>"
                "    <table style='width: 100%; border-collapse: collapse;'>"
                "        <thead>"
                "            <tr style='background: #f8f9fa; border-bottom: 2px solid #dee2e6;'>"
                "                <th style='padding: 1rem; text-align: left;'>Code</th>"
                "                <th style='padding: 1rem; text-align: left;'>Original URL</th>"
                "                <th style='padding: 1rem; text-align: left;'>Clicks</th>"
                "                <th style='padding: 1rem; text-align: left;'>Created</th>"
                "            </tr>"
                "        </thead>"
                "        <tbody>" + urls_table.str() + "</tbody>"
                "    </table>"
                "</div>"
                "<script>"
                "function shortenURL(event) {"
                "    event.preventDefault();"
                "    const longUrl = document.getElementById('longUrl').value;"
                "    const customCode = document.getElementById('customCode').value;"
                "    "
                "    const formData = new URLSearchParams();"
                "    formData.append('url', longUrl);"
                "    formData.append('custom', customCode);"
                "    "
                "    fetch('/api/shorten', {"
                "        method: 'POST',"
                "        headers: {'Content-Type': 'application/x-www-form-urlencoded'},"
                "        body: formData.toString()"
                "    })"
                "    .then(response => response.json())"
                "    .then(data => {"
                "        if (data.status === 'success') {"
                "            document.getElementById('shortUrl').href = data.short_url;"
                "            document.getElementById('shortUrl').textContent = data.short_url;"
                "            document.getElementById('result').style.display = 'block';"
                "            setTimeout(() => window.location.reload(), 2000);"
                "        } else {"
                "            alert('Error: ' + data.message);"
                "        }"
                "    });"
                "    "
                "    return false;"
                "}"
                ""
                "function copyToClipboard() {"
                "    const shortUrl = document.getElementById('shortUrl').textContent;"
                "    navigator.clipboard.writeText(shortUrl).then(() => {"
                "        alert('Copied to clipboard!');"
                "    });"
                "}"
                "</script>";
        
        return generate_generic_app(session_id, "URL Shortener", "🔗",
            "Create and manage shortened URLs", content);
    }
    
    std::string generate_system_monitor_app(const std::string& session_id) {
        system_monitor_.update();
        auto metrics = system_monitor_.get_metrics();
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        
        std::string content = R"HTML(
<style>
    .chart-container {
        background: white;
        padding: 1.5rem;
        border-radius: 8px;
        margin-bottom: 2rem;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    .chart-title {
        font-size: 1.2rem;
        font-weight: bold;
        color: #2c3e50;
        margin-bottom: 1rem;
    }
    .chart-canvas {
        width: 100%;
        height: 200px;
    }
    .stats-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
        gap: 1rem;
        margin-bottom: 2rem;
    }
    .stat-card {
        background: white;
        padding: 1.5rem;
        border-radius: 8px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    .stat-label {
        color: #7f8c8d;
        font-size: 0.9rem;
        margin-bottom: 0.5rem;
    }
    .stat-value {
        font-size: 2rem;
        font-weight: bold;
        margin-bottom: 0.5rem;
    }
    .stat-subtext {
        color: #95a5a6;
        font-size: 0.85rem;
    }
    .connection-status {
        display: inline-flex;
        align-items: center;
        gap: 0.5rem;
        padding: 0.5rem 1rem;
        background: rgba(255, 255, 255, 0.15);
        border-radius: 20px;
        font-size: 0.9rem;
    }
    .status-dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
        animation: pulse 2s ease-in-out infinite;
    }
    .status-connected {
        background: #27ae60;
        box-shadow: 0 0 10px #27ae60;
    }
    .status-disconnected {
        background: #e74c3c;
        box-shadow: 0 0 10px #e74c3c;
    }
    .status-connecting {
        background: #f39c12;
        box-shadow: 0 0 10px #f39c12;
    }
    @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.5; }
    }
</style>

<div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 2rem; border-radius: 8px; margin-bottom: 2rem;">
    <div style="display: flex; justify-content: space-between; align-items: flex-start; flex-wrap: wrap; gap: 1rem;">
        <div>
            <h2 style="margin: 0 0 0.5rem 0;">📊 System Monitor</h2>
            <p style="margin: 0; opacity: 0.9;">Real-time performance metrics with live charts</p>
        </div>
        <div class="connection-status" id="connection-indicator">
            <span class="status-dot status-connecting"></span>
            <span id="connection-text">Connecting...</span>
        </div>
    </div>
    <div style="margin-top: 1rem; font-size: 0.9rem; opacity: 0.8;">
        Auto-refreshing every 2 seconds • <span id="update-status">Active</span>
    </div>
</div>

<div class="stats-grid">
    <div class="stat-card">
        <div class="stat-label">CPU Usage</div>
        <div class="stat-value" style="color: #3498db;"><span id="cpu-current">0</span>%</div>
        <div class="stat-subtext">Current load</div>
    </div>
    <div class="stat-card">
        <div class="stat-label">Memory Usage</div>
        <div class="stat-value" style="color: #9b59b6;"><span id="mem-current">0</span>%</div>
        <div class="stat-subtext"><span id="mem-detail">0 / 0 GB</span></div>
    </div>
    <div class="stat-card">
        <div class="stat-label">Disk Usage</div>
        <div class="stat-value" style="color: #e67e22;"><span id="disk-current">0</span>%</div>
        <div class="stat-subtext"><span id="disk-detail">0 / 0 GB</span></div>
    </div>
    <div class="stat-card">
        <div class="stat-label">Network Activity</div>
        <div class="stat-value" style="color: #27ae60;">
            <span style="font-size: 1rem;">↓</span> <span id="net-rx">0</span> 
            <span style="font-size: 1rem;">↑</span> <span id="net-tx">0</span>
        </div>
        <div class="stat-subtext">Mbps</div>
    </div>
</div>

<div class="chart-container">
    <div class="chart-title">CPU Usage Over Time</div>
    <canvas id="cpu-chart" class="chart-canvas"></canvas>
</div>

<div class="chart-container">
    <div class="chart-title">Memory Usage Over Time</div>
    <canvas id="memory-chart" class="chart-canvas"></canvas>
</div>

<div class="chart-container">
    <div class="chart-title">Network Activity</div>
    <canvas id="network-chart" class="chart-canvas"></canvas>
</div>

<div class="chart-container">
    <div class="chart-title">System Logs & Service Output</div>
    <div id="console-output" style="background: #1e1e1e; color: #d4d4d4; font-family: 'Courier New', monospace; font-size: 0.85rem; padding: 1rem; border-radius: 4px; height: 200px; overflow-y: auto; line-height: 1.5;">
        <div style="color: #4ec9b0;">System Monitor initialized...</div>
        <div style="color: #9cdcfe;">Waiting for data...</div>
    </div>
</div>

<div style="text-align: center; margin-top: 2rem;">
    <button id="pause-btn" onclick="toggleUpdates()" 
            style="background: #e74c3c; color: white; border: none; padding: 0.75rem 2rem; border-radius: 4px; cursor: pointer; font-size: 1rem; margin-right: 1rem;">
        &#9208;&#65039; Pause Updates
    </button>
    <button onclick="clearCharts()" 
            style="background: #95a5a6; color: white; border: none; padding: 0.75rem 2rem; border-radius: 4px; cursor: pointer; font-size: 1rem;">
        &#128465;&#65039; Clear History
    </button>
</div>

<script>
// Chart data storage
const maxDataPoints = 60; // Keep last 60 data points (2 minutes at 2-second intervals)
const chartData = {
    cpu: [],
    memory: [],
    networkRx: [],
    networkTx: [],
    labels: []
};

let updateInterval = null;
let isPaused = false;

// Initialize canvases
const cpuCanvas = document.getElementById('cpu-chart');
const memCanvas = document.getElementById('memory-chart');
const netCanvas = document.getElementById('network-chart');

const cpuCtx = cpuCanvas.getContext('2d');
const memCtx = memCanvas.getContext('2d');
const netCtx = netCanvas.getContext('2d');

// Set canvas sizes
function resizeCanvases() {
    [cpuCanvas, memCanvas, netCanvas].forEach(canvas => {
        canvas.width = canvas.offsetWidth * window.devicePixelRatio;
        canvas.height = canvas.offsetHeight * window.devicePixelRatio;
        const ctx = canvas.getContext('2d');
        ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    });
}
resizeCanvases();
window.addEventListener('resize', resizeCanvases);

// Draw line chart
function drawChart(ctx, canvas, data, color, label, max = 100) {
    const width = canvas.offsetWidth;
    const height = canvas.offsetHeight;
    const padding = 40;
    const chartWidth = width - padding * 2;
    const chartHeight = height - padding * 2;
    
    ctx.clearRect(0, 0, width, height);
    
    // Draw grid
    ctx.strokeStyle = '#ecf0f1';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 5; i++) {
        const y = padding + (chartHeight / 5) * i;
        ctx.beginPath();
        ctx.moveTo(padding, y);
        ctx.lineTo(width - padding, y);
        ctx.stroke();
        
        // Y-axis labels
        ctx.fillStyle = '#7f8c8d';
        ctx.font = '12px Arial';
        ctx.textAlign = 'right';
        ctx.fillText((max - (max / 5) * i).toFixed(0), padding - 10, y + 4);
    }
    
    if (data.length < 2) return;
    
    // Draw line
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    data.forEach((value, index) => {
        const x = padding + (chartWidth / (maxDataPoints - 1)) * index;
        const y = padding + chartHeight - (value / max) * chartHeight;
        
        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });
    
    ctx.stroke();
    
    // Draw filled area
    ctx.lineTo(padding + (chartWidth / (maxDataPoints - 1)) * (data.length - 1), padding + chartHeight);
    ctx.lineTo(padding, padding + chartHeight);
    ctx.closePath();
    
    const gradient = ctx.createLinearGradient(0, padding, 0, height - padding);
    gradient.addColorStop(0, color + '40');
    gradient.addColorStop(1, color + '00');
    ctx.fillStyle = gradient;
    ctx.fill();
    
    // Draw points
    ctx.fillStyle = color;
    data.forEach((value, index) => {
        const x = padding + (chartWidth / (maxDataPoints - 1)) * index;
        const y = padding + chartHeight - (value / max) * chartHeight;
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI * 2);
        ctx.fill();
    });
    
    // Draw current value
    if (data.length > 0) {
        const lastValue = data[data.length - 1];
        ctx.fillStyle = color;
        ctx.font = 'bold 14px Arial';
        ctx.textAlign = 'left';
        ctx.fillText(label + ': ' + lastValue.toFixed(1), padding, 20);
    }
}

// Fetch and update metrics
function updateMetrics() {
    fetch('http://localhost:9001/api/metrics')
        .then(res => {
            if (!res.ok) throw new Error('HTTP ' + res.status);
            return res.json();
        })
        .then(data => {
            // Update connection status to connected
            updateConnectionStatus('connected');
            
            // Log to console
            const timestamp = new Date().toLocaleTimeString();
            logToConsole('info', timestamp + ' - Metrics updated: CPU ' + data.cpu.toFixed(1) + '%, Memory ' + data.memory.toFixed(1) + '%');
            
            // Update stat cards
            document.getElementById('cpu-current').textContent = data.cpu.toFixed(1);
            document.getElementById('mem-current').textContent = data.memory.toFixed(1);
            document.getElementById('disk-current').textContent = data.disk.toFixed(1);
            document.getElementById('net-rx').textContent = data.network_rx.toFixed(1);
            document.getElementById('net-tx').textContent = data.network_tx.toFixed(1);
            
            document.getElementById('mem-detail').textContent = 
                (data.memory * 16 / 100 / 1024).toFixed(1) + ' / 16 GB (est)';
            document.getElementById('disk-detail').textContent = 
                (data.disk * 500 / 100).toFixed(1) + ' / 500 GB (est)';
            
            // Add to chart data
            chartData.cpu.push(data.cpu);
            chartData.memory.push(data.memory);
            chartData.networkRx.push(data.network_rx);
            chartData.networkTx.push(data.network_tx);
            
            const time = new Date().toLocaleTimeString();
            chartData.labels.push(time);
            
            // Keep only last maxDataPoints
            if (chartData.cpu.length > maxDataPoints) {
                chartData.cpu.shift();
                chartData.memory.shift();
                chartData.networkRx.shift();
                chartData.networkTx.shift();
                chartData.labels.shift();
            }
            
            // Redraw charts
            drawChart(cpuCtx, cpuCanvas, chartData.cpu, '#3498db', 'CPU', 100);
            drawChart(memCtx, memCanvas, chartData.memory, '#9b59b6', 'Memory', 100);
            
            // Network chart with two lines
            const netMax = Math.max(
                Math.max(...chartData.networkRx, 10),
                Math.max(...chartData.networkTx, 10)
            );
            drawChart(netCtx, netCanvas, chartData.networkRx, '#27ae60', '↓ Download', netMax);
            
            // Draw upload line on same chart
            const ctx = netCtx;
            const canvas = netCanvas;
            const data = chartData.networkTx;
            const color = '#e74c3c';
            const width = canvas.offsetWidth;
            const height = canvas.offsetHeight;
            const padding = 40;
            const chartWidth = width - padding * 2;
            const chartHeight = height - padding * 2;
            
            if (data.length >= 2) {
                ctx.strokeStyle = color;
                ctx.lineWidth = 2;
                ctx.beginPath();
                
                data.forEach((value, index) => {
                    const x = padding + (chartWidth / (maxDataPoints - 1)) * index;
                    const y = padding + chartHeight - (value / netMax) * chartHeight;
                    
                    if (index === 0) {
                        ctx.moveTo(x, y);
                    } else {
                        ctx.lineTo(x, y);
                    }
                });
                
                ctx.stroke();
                
                // Draw points
                ctx.fillStyle = color;
                data.forEach((value, index) => {
                    const x = padding + (chartWidth / (maxDataPoints - 1)) * index;
                    const y = padding + chartHeight - (value / netMax) * chartHeight;
                    ctx.beginPath();
                    ctx.arc(x, y, 3, 0, Math.PI * 2);
                    ctx.fill();
                });
                
                // Draw current value
                const lastValue = data[data.length - 1];
                ctx.fillStyle = color;
                ctx.font = 'bold 14px Arial';
                ctx.textAlign = 'left';
                ctx.fillText('↑ Upload: ' + lastValue.toFixed(1), padding + 150, 20);
            }
        })
        .catch(err => {
            console.error('Failed to fetch metrics:', err);
            updateConnectionStatus('disconnected');
            logToConsole('error', 'Connection failed: ' + err.message);
            document.getElementById('update-status').textContent = 'Error';
        });
}

function logToConsole(level, message) {
    const consoleOutput = document.getElementById('console-output');
    if (!consoleOutput) return;
    
    const colors = {
        'info': '#9cdcfe',
        'success': '#4ec9b0',
        'warning': '#dcdcaa',
        'error': '#f48771'
    };
    
    const entry = document.createElement('div');
    entry.style.color = colors[level] || colors['info'];
    entry.textContent = message;
    
    consoleOutput.appendChild(entry);
    
    // Keep only last 50 lines
    while (consoleOutput.children.length > 50) {
        consoleOutput.removeChild(consoleOutput.firstChild);
    }
    
    // Auto-scroll to bottom
    consoleOutput.scrollTop = consoleOutput.scrollHeight;
}

function updateConnectionStatus(status) {
    const indicator = document.querySelector('#connection-indicator .status-dot');
    const text = document.getElementById('connection-text');
    
    indicator.className = 'status-dot';
    
    if (status === 'connected') {
        indicator.classList.add('status-connected');
        text.textContent = 'Connected';
    } else if (status === 'disconnected') {
        indicator.classList.add('status-disconnected');
        text.textContent = 'Disconnected';
    } else if (status === 'connecting') {
        indicator.classList.add('status-connecting');
        text.textContent = 'Connecting...';
    }
}

function toggleUpdates() {
    isPaused = !isPaused;
    const btn = document.getElementById('pause-btn');
    
    if (isPaused) {
        clearInterval(updateInterval);
        btn.innerHTML = '▶️ Resume Updates';
        btn.style.background = '#27ae60';
        document.getElementById('update-status').textContent = 'Paused';
    } else {
        updateInterval = setInterval(updateMetrics, 2000);
        btn.innerHTML = '⏸️ Pause Updates';
        btn.style.background = '#e74c3c';
        document.getElementById('update-status').textContent = 'Active';
    }
}

function clearCharts() {
    chartData.cpu = [];
    chartData.memory = [];
    chartData.networkRx = [];
    chartData.networkTx = [];
    chartData.labels = [];
    
    cpuCtx.clearRect(0, 0, cpuCanvas.width, cpuCanvas.height);
    memCtx.clearRect(0, 0, memCanvas.width, memCanvas.height);
    netCtx.clearRect(0, 0, netCanvas.width, netCanvas.height);
}

// Start updating
logToConsole('success', 'System Monitor started');
logToConsole('info', 'Connecting to backend service...');
updateConnectionStatus('connecting');
updateMetrics();
updateInterval = setInterval(updateMetrics, 2000);
</script>
)HTML";
        
        return generate_generic_app(session_id, "System Monitor", "📊",
            "Real-time system performance metrics", content);
    }
    
    std::string generate_service_breaker_app(const std::string& session_id) {
        auto services = service_breaker_.get_all_services();
        
        std::string content = R"(
<style>
    .service-card {
        background: white;
        padding: 1.5rem;
        margin-bottom: 1.5rem;
        border-left: 4px solid #3498db;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    .service-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 1rem;
    }
    .service-name {
        font-size: 1.3rem;
        font-weight: bold;
        color: #2c3e50;
    }
    .status-badge {
        padding: 0.5rem 1rem;
        border-radius: 4px;
        font-weight: bold;
        font-size: 0.9rem;
    }
    .status-running {
        background: #27ae60;
        color: white;
    }
    .status-stopped {
        background: #e74c3c;
        color: white;
    }
    .service-controls {
        display: flex;
        gap: 1rem;
        margin-top: 1rem;
        flex-wrap: wrap;
    }
    .btn {
        padding: 0.75rem 1.5rem;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-size: 0.95rem;
        font-weight: 500;
        transition: all 0.3s;
    }
    .btn-start {
        background: #27ae60;
        color: white;
    }
    .btn-start:hover {
        background: #229954;
    }
    .btn-stop {
        background: #e74c3c;
        color: white;
    }
    .btn-stop:hover {
        background: #c0392b;
    }
    .port-input {
        padding: 0.75rem;
        border: 1px solid #ddd;
        border-radius: 4px;
        width: 150px;
        font-size: 0.95rem;
    }
    .btn:disabled {
        opacity: 0.6;
        cursor: not-allowed;
    }
    .spinner {
        display: inline-block;
        width: 14px;
        height: 14px;
        border: 2px solid rgba(255,255,255,0.3);
        border-radius: 50%;
        border-top-color: white;
        animation: spin 0.8s linear infinite;
        margin-right: 0.5rem;
        vertical-align: middle;
    }
    @keyframes spin {
        to { transform: rotate(360deg); }
    }
    .status-message {
        position: fixed;
        top: 2rem;
        right: 2rem;
        background: white;
        padding: 1rem 1.5rem;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.15);
        z-index: 9999;
        animation: slideIn 0.3s ease-out;
    }
    @keyframes slideIn {
        from { transform: translateX(400px); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }
</style>

<div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 2rem; border-radius: 8px; margin-bottom: 2rem; color: white;">
    <h2 style="margin: 0; font-size: 2rem;">🔧 Service Breaker</h2>
    <p style="margin: 0.5rem 0 0 0; opacity: 0.9;">Control and configure all platform services</p>
</div>

<div id="services-container">
)";
        
        // Generate cards for each service
        for (const auto& [name, config] : services) {
            content += "<div class='service-card'>";
            content += "<div class='service-header'>";
            content += "<div><div class='service-name'>" + config.name + "</div>";
            content += "<div style='color: #7f8c8d; margin-top: 0.25rem;'>" + config.description + "</div></div>";
            content += "<div class='status-badge status-" + std::string(config.is_running ? "running" : "stopped") + "'>";
            content += config.is_running ? "🟢 Running" : "🔴 Stopped";
            content += "</div></div>";
            
            content += "<div style='display: flex; gap: 2rem; align-items: center; margin-top: 1rem; flex-wrap: wrap;'>";
            content += "<div><strong>Port:</strong> <input type='number' id='port-" + name + "' value='" + std::to_string(config.port) + "' class='port-input' min='1' max='65535'></div>";
            content += "<div><strong>Status:</strong> <span style='color: #7f8c8d;'>" + config.status_message + "</span></div>";
            content += "</div>";
            
            content += "<div class='service-controls'>";
            content += "<button class='btn btn-start' onclick='startService(\"" + name + "\")'>▶️ Start</button>";
            content += "<button class='btn btn-stop' onclick='stopService(\"" + name + "\")'>⏹️ Stop</button>";
            content += "<button class='btn' style='background: #3498db; color: white;' onclick='setPort(\"" + name + "\")'>💾 Set Port</button>";
            content += "</div></div>";
        }
        
        content += R"(
</div>

<script>
function showToast(message, type) {
    const toast = document.createElement('div');
    toast.className = 'status-message';
    toast.style.borderLeft = '4px solid ' + (type === 'success' ? '#27ae60' : '#e74c3c');
    toast.innerHTML = '<strong>' + (type === 'success' ? '✓' : '✗') + '</strong> ' + message;
    document.body.appendChild(toast);
    setTimeout(() => {
        toast.style.animation = 'slideIn 0.3s ease-out reverse';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

function setButtonLoading(button, loading) {
    if (loading) {
        button.disabled = true;
        button.setAttribute('data-original-text', button.innerHTML);
        button.innerHTML = '<span class="spinner"></span>' + button.getAttribute('data-action') + 'ing...';
    } else {
        button.disabled = false;
        button.innerHTML = button.getAttribute('data-original-text');
    }
}

function startService(serviceName) {
    const button = event.target;
    button.setAttribute('data-action', 'Start');
    setButtonLoading(button, true);
    
    fetch('/api/service/start', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'service=' + serviceName
    })
    .then(res => res.json())
    .then(data => {
        if (data.status === 'success') {
            showToast('Service ' + serviceName + ' started successfully', 'success');
            setTimeout(() => window.location.reload(), 1000);
        } else {
            showToast('Failed to start service ' + serviceName, 'error');
            setButtonLoading(button, false);
        }
    })
    .catch(err => {
        showToast('Error starting service: ' + err.message, 'error');
        setButtonLoading(button, false);
    });
}

function stopService(serviceName) {
    const button = event.target;
    button.setAttribute('data-action', 'Stop');
    setButtonLoading(button, true);
    
    fetch('/api/service/stop', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'service=' + serviceName
    })
    .then(res => res.json())
    .then(data => {
        if (data.status === 'success') {
            showToast('Service ' + serviceName + ' stopped successfully', 'success');
            setTimeout(() => window.location.reload(), 1000);
        } else {
            showToast('Failed to stop service ' + serviceName, 'error');
            setButtonLoading(button, false);
        }
    })
    .catch(err => {
        showToast('Error stopping service: ' + err.message, 'error');
        setButtonLoading(button, false);
    });
}

function setPort(serviceName) {
    const button = event.target;
    const port = document.getElementById('port-' + serviceName).value;
    
    if (port < 1 || port > 65535) {
        showToast('Invalid port number. Must be between 1 and 65535.', 'error');
        return;
    }
    
    button.setAttribute('data-action', 'Sav');
    setButtonLoading(button, true);
    
    fetch('/api/service/setport', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'service=' + serviceName + '&port=' + port
    })
    .then(res => res.json())
    .then(data => {
        if (data.status === 'success') {
            showToast('Port for ' + serviceName + ' set to ' + port, 'success');
            setTimeout(() => window.location.reload(), 1000);
        } else {
            showToast('Failed to set port for ' + serviceName, 'error');
            setButtonLoading(button, false);
        }
    })
    .catch(err => {
        showToast('Error setting port: ' + err.message, 'error');
        setButtonLoading(button, false);
    });
}
</script>
)";
        
        return generate_generic_app(session_id, "Service Breaker", "🔧",
            "Control and configure all platform services", content);
    }
    
    std::string generate_admin_panel(const std::string& session_id) {
        
        auto users = auth_system_->list_users();
        std::stringstream users_html;
        for (const auto& username : users) {
            auto* user = auth_system_->get_user(username);
            if (user) {
                users_html << "<tr><td>" << username << "</td>"
                          << "<td>" << user->email << "</td>"
                          << "<td>" << role_to_string(user->role) << "</td>"
                          << "<td style='color: " << (user->is_active ? "green" : "red") << ";'>" 
                          << (user->is_active ? "✓ Active" : "✗ Inactive") << "</td></tr>";
            }
        }
        
        std::string content = 
            "<h3>Server Control</h3>"
            "<div style=\"background: white; padding: 1.5rem; border-radius: 8px; margin-bottom: 2rem;\">"
            "    <p style=\"margin-bottom: 1rem;\"><strong>Account Server Status:</strong> "
            "    <span id=\"server-status\" style=\"color: orange;\">⚠ Not Running</span></p>"
            "    <button onclick=\"startAccountServer()\" "
            "        style=\"background: #27ae60; color: white; border: none; "
            "               padding: 0.75rem 1.5rem; border-radius: 4px; cursor: pointer; font-size: 1rem;\">"
            "        🚀 Start Account Server"
            "    </button>"
            "</div>"
            
            "<h3>Create New User</h3>"
            "<form id=\"createUserForm\" onsubmit=\"return createUser(event);\" "
            "      style=\"background: white; padding: 1.5rem; border-radius: 8px; margin-bottom: 2rem;\">"
            "    <div style=\"margin-bottom: 1rem;\">"
            "        <label style=\"display: block; margin-bottom: 0.5rem; font-weight: bold;\">Username</label>"
            "        <input type=\"text\" id=\"username\" required placeholder=\"newuser\" "
            "               style=\"width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;\">"
            "    </div>"
            "    <div style=\"margin-bottom: 1rem;\">"
            "        <label style=\"display: block; margin-bottom: 0.5rem; font-weight: bold;\">Email</label>"
            "        <input type=\"email\" id=\"email\" required placeholder=\"user@example.com\" "
            "               style=\"width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;\">"
            "    </div>"
            "    <div style=\"margin-bottom: 1rem;\">"
            "        <label style=\"display: block; margin-bottom: 0.5rem; font-weight: bold;\">Password</label>"
            "        <input type=\"password\" id=\"password\" required placeholder=\"password123\" "
            "               style=\"width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;\">"
            "    </div>"
            "    <div style=\"margin-bottom: 1rem;\">"
            "        <label style=\"display: block; margin-bottom: 0.5rem; font-weight: bold;\">Role</label>"
            "        <select id=\"role\" style=\"width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem;\">"
            "            <option value=\"user\">User</option>"
            "            <option value=\"admin\">Admin</option>"
            "        </select>"
            "    </div>"
            "    <button type=\"submit\" style=\"background: #3498db; color: white; border: none; "
            "                                    padding: 0.75rem 2rem; border-radius: 4px; cursor: pointer; font-size: 1rem;\">"
            "        ➕ Create User"
            "    </button>"
            "</form>"
            
            "<h3>Existing Users</h3>"
            "<div style=\"background: white; padding: 1.5rem; border-radius: 8px; margin-bottom: 2rem; overflow-x: auto;\">"
            "    <table style=\"width: 100%; border-collapse: collapse;\">"
            "        <thead>"
            "            <tr style=\"background: #f8f9fa; border-bottom: 2px solid #dee2e6;\">"
            "                <th style=\"padding: 1rem; text-align: left; font-weight: bold;\">Username</th>"
            "                <th style=\"padding: 1rem; text-align: left; font-weight: bold;\">Email</th>"
            "                <th style=\"padding: 1rem; text-align: left; font-weight: bold;\">Role</th>"
            "                <th style=\"padding: 1rem; text-align: left; font-weight: bold;\">Status</th>"
            "            </tr>"
            "        </thead>"
            "        <tbody id=\"users-table\">"
            + users_html.str() +
            "        </tbody>"
            "    </table>"
            "</div>"
            
            "<h3>System Statistics</h3>"
            "<div style=\"background: white; padding: 1.5rem; border-radius: 8px;\">"
            "    <p style=\"margin-bottom: 0.5rem;\"><strong>Active Sessions:</strong> " + 
                    std::to_string(auth_system_->get_active_sessions_count()) + "</p>"
            "    <p style=\"margin-bottom: 0.5rem;\"><strong>Total Users:</strong> " + 
                    std::to_string(auth_system_->get_total_users()) + "</p>"
            "    <p style=\"margin-bottom: 0.5rem;\"><strong>Platform:</strong> ToolBox MATLAB-Style</p>"
            "    <p style=\"margin-bottom: 0.5rem;\"><strong>Port:</strong> 9000</p>"
            "</div>"
            
            "<script>"
            "function startAccountServer() {"
            "    fetch('/api/start_account_server')"
            "        .then(response => response.json())"
            "        .then(data => {"
            "            document.getElementById('server-status').innerHTML = '✓ Running on port 8888';"
            "            document.getElementById('server-status').style.color = 'green';"
            "            alert(data.message);"
            "        })"
            "        .catch(err => alert('Error starting server'));"
            "}"
            ""
            "function createUser(event) {"
            "    event.preventDefault();"
            "    const formData = new URLSearchParams();"
            "    formData.append('username', document.getElementById('username').value);"
            "    formData.append('email', document.getElementById('email').value);"
            "    formData.append('password', document.getElementById('password').value);"
            "    formData.append('role', document.getElementById('role').value);"
            "    "
            "    fetch('/api/create_user', {"
            "        method: 'POST',"
            "        headers: {'Content-Type': 'application/x-www-form-urlencoded'},"
            "        body: formData.toString()"
            "    })"
            "    .then(() => window.location.reload())"
            "    .catch(err => alert('Error creating user'));"
            "    "
            "    return false;"
            "}"
            "</script>";
        
        return generate_generic_app(session_id, "Admin Panel", "⚙️",
            "System and user administration", content);
    }
    
    std::string generate_account_management(const std::string& session_id) {
        // Get current user info (using default guest for passwordless mode)
        std::string username = "Guest";
        std::string email = "guest@toolbox.local";
        std::string role = "User";
        std::string status = "Active";
        
        // Get system preferences
        std::time_t now = std::time(nullptr);
        std::tm* local_time = std::localtime(&now);
        char time_buffer[64];
        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
        
        return generate_generic_app(session_id, "Account Management", "👤",
            "Manage your profile and preferences",
            R"(
                <h3>Profile Information</h3>
                <form-input label="Username" value=")" + username + R"(" placeholder="username"></form-input>
                <form-input label="Email" value=")" + email + R"(" placeholder="user@example.com"></form-input>
                <form-input label="Display Name" placeholder="Full Name"></form-input>
                <app-button>Update Profile</app-button>
                
                <h3 style="margin-top: 2rem;">Account Status</h3>
                <div style="background: white; padding: 1.5rem; border-radius: 8px; margin-bottom: 1rem;">
                    <p><strong>Role:</strong> )" + role + R"(</p>
                    <p><strong>Status:</strong> )" + status + R"(</p>
                    <p><strong>Last Login:</strong> )" + std::string(time_buffer) + R"(</p>
                    <p><strong>Sessions:</strong> )" + std::to_string(auth_system_->get_active_sessions_count()) + R"( active</p>
                </div>
                
                <h3 style="margin-top: 2rem;">Security Settings</h3>
                <form-input label="Current Password" type="password" placeholder="••••••••"></form-input>
                <form-input label="New Password" type="password" placeholder="••••••••"></form-input>
                <form-input label="Confirm Password" type="password" placeholder="••••••••"></form-input>
                <app-button>Change Password</app-button>
                
                <h3 style="margin-top: 2rem;">Preferences</h3>
                <div style="background: white; padding: 1.5rem; border-radius: 8px; margin-bottom: 1rem;">
                    <label style="display: block; margin-bottom: 1rem;">
                        <input type="checkbox" checked> Enable email notifications
                    </label>
                    <label style="display: block; margin-bottom: 1rem;">
                        <input type="checkbox"> Dark mode
                    </label>
                    <label style="display: block; margin-bottom: 1rem;">
                        <input type="checkbox" checked> Show system statistics
                    </label>
                </div>
                <app-button>Save Preferences</app-button>
                
                <h3 style="margin-top: 2rem;">Account Actions</h3>
                <div style="display: flex; gap: 1rem; flex-wrap: wrap;">
                    <app-button>Export Data</app-button>
                    <app-button>Clear Sessions</app-button>
                    <app-button style="background: #e74c3c;">Delete Account</app-button>
                </div>
            )");
    }
    
    std::string generate_generic_app(const std::string& session_id, const std::string& app_name,
                                    const std::string& icon, const std::string& description,
                                    const std::string& content) {
        
        ComponentBundler bundler;
        
        return bundler
            .set_title(app_name + " - MATLAB Style")
            .add_component_from_registry("app-button")
            .add_component_from_registry("form-input")
            .add_component_from_registry("progress-bar")
            .add_component_from_registry("data-table")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: Arial, Helvetica, sans-serif; background: #f0f0f0; }
                .app-header {
                    background: #2c3e50;
                    color: white;
                    padding: 1rem 2rem;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                }
                .app-header h1 { font-size: 1.5rem; color: white; }
                .back-btn {
                    background: #34495e;
                    color: white;
                    padding: 0.5rem 1rem;
                    border-radius: 4px;
                    text-decoration: none;
                }
                .app-container {
                    max-width: 1200px;
                    margin: 2rem auto;
                    background: white;
                    border-radius: 8px;
                    padding: 2rem;
                    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
                }
                .app-icon { font-size: 3rem; margin-bottom: 1rem; }
                h2 { color: #2c3e50; margin-bottom: 0.5rem; }
                h3 { color: #34495e; margin: 1.5rem 0 1rem; }
                p { color: #7f8c8d; margin: 0.5rem 0; }
                .version-footer {
                    position: fixed;
                    bottom: 1rem;
                    right: 1rem;
                    background: rgba(255, 255, 255, 0.9);
                    padding: 0.5rem 1rem;
                    border-radius: 4px;
                    box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                    font-size: 0.8rem;
                    color: #7f8c8d;
                    z-index: 1000;
                }
                .version-footer strong {
                    color: #2c3e50;
                }
            )")
            .set_body_content(R"(
                <div class="app-header">
                    <h1>)" + icon + " " + app_name + R"(</h1>
                    <a href="/" class="back-btn">← Back to Apps</a>
                </div>
                <div class="app-container">
                    <div class="app-icon">)" + icon + R"(</div>
                    <h2>)" + app_name + R"(</h2>
                    <p>)" + description + R"(</p>
                    <hr style="margin: 2rem 0; border: 1px solid #ecf0f1;">
                    )" + content + R"(
                </div>
                
                <div class="version-footer">
                    <strong>ToolBox Platform</strong> v1.0.0
                </div>
            )")
            .minify(true)
            .bundle();
    }
};

#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <atomic>

struct ServiceInfo {
    std::string name;
    std::string command;
    int port;
    pid_t pid;
    bool running;
    std::string description;
    std::vector<std::string> output_lines;
    int output_fd;
};

class ServiceManager {
private:
    std::map<int, ServiceInfo> services_;
    std::mutex mutex_;
    std::atomic<bool> running_{true};
    std::thread output_reader_;
    
    void read_service_outputs() {
        fd_set read_fds;
        struct timeval tv;
        char buffer[4096];
        
        while (running_) {
            FD_ZERO(&read_fds);
            int max_fd = 0;
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& [id, svc] : services_) {
                    if (svc.running && svc.output_fd > 0) {
                        FD_SET(svc.output_fd, &read_fds);
                        max_fd = std::max(max_fd, svc.output_fd);
                    }
                }
            }
            
            if (max_fd == 0) {
                usleep(100000);
                continue;
            }
            
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            
            int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
            if (result > 0) {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& [id, svc] : services_) {
                    if (svc.running && svc.output_fd > 0 && FD_ISSET(svc.output_fd, &read_fds)) {
                        ssize_t n = read(svc.output_fd, buffer, sizeof(buffer) - 1);
                        if (n > 0) {
                            buffer[n] = '\0';
                            std::string output(buffer);
                            
                            // Split by newlines and add to output_lines
                            size_t start = 0;
                            size_t end = output.find('\n');
                            while (end != std::string::npos) {
                                std::string line = output.substr(start, end - start);
                                if (!line.empty()) {
                                    svc.output_lines.push_back(line);
                                    // Keep last 100 lines
                                    if (svc.output_lines.size() > 100) {
                                        svc.output_lines.erase(svc.output_lines.begin());
                                    }
                                }
                                start = end + 1;
                                end = output.find('\n', start);
                            }
                            // Add remaining part
                            if (start < output.length()) {
                                std::string line = output.substr(start);
                                if (!line.empty()) {
                                    svc.output_lines.push_back(line);
                                    if (svc.output_lines.size() > 100) {
                                        svc.output_lines.erase(svc.output_lines.begin());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
public:
    ServiceManager() {
        services_[1] = {"Frontend", "", 9000, -1, true, "MATLAB-Style Web UI (this process)", {}, -1};
        services_[2] = {"Metrics Backend", "./services/build/metrics_backend_service 9001", 9001, -1, false, "System metrics API", {}, -1};
        services_[3] = {"Account Service", "./services/build/account_service 9002", 9002, -1, false, "User account management", {}, -1};
        
        // Start output reading thread
        output_reader_ = std::thread(&ServiceManager::read_service_outputs, this);
    }
    
    ~ServiceManager() {
        running_ = false;
        if (output_reader_.joinable()) {
            output_reader_.join();
        }
    }
    
    bool start_service(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (services_.find(id) == services_.end()) return false;
        if (id == 1) return true; // Frontend always running
        
        ServiceInfo& svc = services_[id];
        if (svc.running) return true;
        
        // Create pipe for capturing output
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            return false;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(pipefd[0]); // Close read end
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            
            setsid(); // Create new session
            execl("/bin/sh", "sh", "-c", svc.command.c_str(), nullptr);
            exit(1); // If exec fails
        } else if (pid > 0) {
            close(pipefd[1]); // Close write end
            
            // Make read end non-blocking
            int flags = fcntl(pipefd[0], F_GETFL, 0);
            fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
            
            svc.pid = pid;
            svc.output_fd = pipefd[0];
            svc.running = true;
            svc.output_lines.clear();
            usleep(100000); // Give service time to start
            return true;
        }
        
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }
    
    bool stop_service(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (services_.find(id) == services_.end()) return false;
        if (id == 1) return false; // Can't stop frontend
        
        ServiceInfo& svc = services_[id];
        if (!svc.running) return true;
        
        if (svc.pid > 0) {
            kill(-svc.pid, SIGTERM); // Kill process group
            waitpid(svc.pid, nullptr, WNOHANG);
            svc.pid = -1;
        }
        
        if (svc.output_fd > 0) {
            close(svc.output_fd);
            svc.output_fd = -1;
        }
        
        svc.running = false;
        return true;
    }
    
    bool restart_service(int id) {
        stop_service(id);
        usleep(200000);
        return start_service(id);
    }
    
    std::map<int, ServiceInfo> get_services() {
        std::lock_guard<std::mutex> lock(mutex_);
        return services_;
    }
    
    std::vector<std::string> get_service_output(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (services_.find(id) != services_.end()) {
            return services_[id].output_lines;
        }
        return {};
    }
    
    void stop_all() {
        for (auto& [id, svc] : services_) {
            if (id != 1) stop_service(id);
        }
    }
};

void print_service_tui(ServiceManager& manager) {
    std::cout << "\033[2J\033[H"; // Clear screen
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           🚀 MATLAB-Style Platform - Service Manager 🚀              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
    
    auto services = manager.get_services();
    
    std::cout << "┌────┬─────────────────────────┬──────┬──────────┬───────────────────────┐\n";
    std::cout << "│ ID │ SERVICE NAME            │ PORT │ STATUS   │ DESCRIPTION           │\n";
    std::cout << "├────┼─────────────────────────┼──────┼──────────┼───────────────────────┤\n";
    
    for (const auto& [id, svc] : services) {
        std::string status = svc.running ? "\033[32m● RUN \033[0m" : "\033[31m○ STOP\033[0m";
        printf("│ %2d │ %-23s │ %4d │ %s │ %-21s │\n", 
               id, svc.name.c_str(), svc.port, status.c_str(), svc.description.c_str());
    }
    
    std::cout << "└────┴─────────────────────────┴──────┴──────────┴───────────────────────┘\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                              COMMANDS                                 ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  \033[1m[1-3]\033[0m      - Start service by ID                                     ║\n";
    std::cout << "║  \033[1ms [1-3]\033[0m   - Stop service by ID                                      ║\n";
    std::cout << "║  \033[1mr [1-3]\033[0m   - Restart service by ID                                   ║\n";
    std::cout << "║  \033[1mlog [1-3]\033[0m - View service output logs                                ║\n";
    std::cout << "║  \033[1mrefresh\033[0m   - Refresh status display                                  ║\n";
    std::cout << "║  \033[1mopen\033[0m      - Open main UI in browser (http://localhost:9000)        ║\n";
    std::cout << "║  \033[1mquit\033[0m      - Stop all services and exit                              ║\n";
    std::cout << "║                                                                       ║\n";
    std::cout << "║  \033[1m➜\033[0m Main UI: \033[4mhttp://localhost:9000\033[0m                                   ║\n";
    std::cout << "║  \033[1m➜\033[0m Login: admin/admin123 or user/user123                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Command: " << std::flush;
}

void show_service_logs(ServiceManager& manager, int id) {
    auto services = manager.get_services();
    if (services.find(id) == services.end()) {
        std::cout << "\n\033[31m✗ Service " << id << " not found\033[0m\n";
        return;
    }
    
    auto output = manager.get_service_output(id);
    auto& svc = services[id];
    
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              📋 Service Logs - " << svc.name;
    for (int i = svc.name.length(); i < 42; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "\033[90m┌" << std::string(73, '-') << "┐\033[0m\n";
    
    if (output.empty()) {
        std::cout << "\033[90m│\033[0m \033[33mNo output captured yet...\033[0m";
        for (int i = 26; i < 72; i++) std::cout << " ";
        std::cout << "\033[90m│\033[0m\n";
    } else {
        for (const auto& line : output) {
            std::cout << "\033[90m│\033[0m ";
            if (line.length() > 71) {
                std::cout << line.substr(0, 68) << "...";
            } else {
                std::cout << line;
                for (size_t i = line.length(); i < 71; i++) std::cout << " ";
            }
            std::cout << "\033[90m│\033[0m\n";
        }
    }
    
    std::cout << "\033[90m└" << std::string(73, '-') << "┘\033[0m\n\n";
    
    std::cout << "\033[36mℹ️  Showing last " << output.size() << " lines (max 100)\033[0m\n";
    std::cout << "\033[90mPress Enter to return to main menu...\033[0m" << std::flush;
}

int main() {
    std::cout << "\033[?25h"; // Show cursor
    
    // Create authentication system
    AuthSystem auth_system;
    
    // Register web components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_button());
    registry.register_component(components::create_form_input());
    registry.register_component(components::create_progress_bar());
    registry.register_component(components::create_data_table());
    
    // Create service manager
    ServiceManager service_manager;
    
    // Start MATLAB-style UI in background thread
    std::atomic<bool> ui_running{false};
    std::thread ui_thread([&]() {
        MATLABStyleUI ui(9000, &auth_system);
        ui_running = true;
        ui.start();
    });
    
    // Wait for UI to start
    while (!ui_running) {
        usleep(100000);
    }
    
    usleep(500000); // Give UI time to bind
    
    // Interactive TUI loop
    print_service_tui(service_manager);
    
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit" || input == "q" || input == "exit") {
            std::cout << "\n\033[33mStopping all services...\033[0m\n";
            service_manager.stop_all();
            std::cout << "\033[32m✓ All services stopped\033[0m\n";
            std::cout << "Goodbye!\n";
            break;
        } else if (input == "refresh" || input.empty()) {
            print_service_tui(service_manager);
        } else if (input == "open") {
            std::cout << "\n\033[36m⚡ Opening browser...\033[0m\n";
            system("open http://localhost:9000 2>/dev/null || xdg-open http://localhost:9000 2>/dev/null");
            std::cout << "\033[32m✓ Browser opened at http://localhost:9000\033[0m\n\n";
            std::cout << "\033[90mPress Enter to continue...\033[0m" << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        } else if (input.length() >= 5 && input.substr(0, 4) == "log ") {
            int id = input[4] - '0';
            show_service_logs(service_manager, id);
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        } else if (input.length() >= 3 && input[0] == 's' && input[1] == ' ') {
            int id = input[2] - '0';
            std::cout << "\n\033[33m⏹️  Stopping service " << id << "...\033[0m\n\n";
            if (service_manager.stop_service(id)) {
                std::cout << "\n\033[32m✓ Service " << id << " stopped successfully\033[0m\n\n";
            } else {
                std::cout << "\n\033[31m✗ Failed to stop service " << id << "\033[0m\n\n";
            }
            std::cout << "\033[90mPress Enter to continue...\033[0m" << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        } else if (input.length() >= 3 && input[0] == 'r' && input[1] == ' ') {
            int id = input[2] - '0';
            std::cout << "\n\033[33m🔄 Restarting service " << id << "...\033[0m\n\n";
            if (service_manager.restart_service(id)) {
                std::cout << "\n\033[32m✓ Service " << id << " restarted successfully\033[0m\n\n";
            } else {
                std::cout << "\n\033[31m✗ Failed to restart service " << id << "\033[0m\n\n";
            }
            std::cout << "\033[90mPress Enter to continue...\033[0m" << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        } else if (input.length() == 1 && isdigit(input[0])) {
            int id = input[0] - '0';
            std::cout << "\n\033[36m▶️  Starting service " << id << "...\033[0m\n";
            std::cout << "\033[90m" << std::string(75, '-') << "\033[0m\n\n";
            if (service_manager.start_service(id)) {
                std::cout << "\n\033[90m" << std::string(75, '-') << "\033[0m\n";
                std::cout << "\033[32m✓ Service " << id << " started successfully\033[0m\n\n";
            } else {
                std::cout << "\n\033[90m" << std::string(75, '-') << "\033[0m\n";
                std::cout << "\033[31m✗ Failed to start service " << id << "\033[0m\n\n";
            }
            std::cout << "\033[90mPress Enter to continue...\033[0m" << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        } else {
            std::cout << "\n\033[31m✗ Unknown command: '" << input << "'\033[0m\n";
            std::cout << "\033[90mType 'quit' to exit, or press Enter for help.\033[0m\n\n";
            std::cout << "\033[90mPress Enter to continue...\033[0m" << std::flush;
            std::string dummy;
            std::getline(std::cin, dummy);
            print_service_tui(service_manager);
        }
    }
    
    // Cleanup
    ui_thread.detach();
    return 0;
}
