#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <mutex>
#include <ctime>

struct UserAccount {
    std::string username;
    std::string password;
    std::string email;
    std::string full_name;
    std::string bio;
    std::string created_at;
    std::string last_login;
};

class AccountService {
private:
    int port_;
    int server_fd_;
    bool running_;
    std::map<std::string, UserAccount> accounts_;
    std::map<std::string, std::string> sessions_; // session_id -> username
    std::mutex accounts_mutex_;

    std::string generate_session_id() {
        return "sess_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand());
    }

    std::string get_current_time() {
        time_t now = time(nullptr);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    std::string parse_post_param(const std::string& body, const std::string& key) {
        size_t pos = body.find(key + "=");
        if (pos == std::string::npos) return "";
        pos += key.length() + 1;
        size_t end = body.find("&", pos);
        if (end == std::string::npos) end = body.length();
        return body.substr(pos, end - pos);
    }

    std::string url_decode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); i++) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int value;
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
                result += static_cast<char>(value);
                i += 2;
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

    std::string get_session_from_cookie(const std::string& request) {
        size_t pos = request.find("Cookie:");
        if (pos == std::string::npos) return "";
        size_t start = request.find("session_id=", pos);
        if (start == std::string::npos) return "";
        start += 11;
        size_t end = request.find_first_of(";\r\n", start);
        if (end == std::string::npos) end = request.length();
        return request.substr(start, end - start);
    }

    std::string http_response(const std::string& body, const std::string& content_type = "text/html", 
                              const std::string& extra_headers = "") {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << content_type << "\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << extra_headers
                 << "Connection: close\r\n"
                 << "\r\n"
                 << body;
        return response.str();
    }

    std::string generate_login_page(const std::string& message = "") {
        std::string msg_html = message.empty() ? "" : 
            "<div style='background: #e74c3c; color: white; padding: 1rem; border-radius: 4px; margin-bottom: 1rem;'>" + message + "</div>";
        
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>Account Service - Login</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 2rem;
        }
        .container {
            background: white;
            padding: 3rem;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 450px;
            width: 100%;
        }
        h1 { color: #2c3e50; margin-bottom: 0.5rem; font-size: 2rem; }
        .subtitle { color: #7f8c8d; margin-bottom: 2rem; }
        .form-group { margin-bottom: 1.5rem; }
        label { display: block; margin-bottom: 0.5rem; color: #2c3e50; font-weight: 500; }
        input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 1rem;
        }
        input:focus { outline: none; border-color: #667eea; }
        .btn {
            width: 100%;
            padding: 0.875rem;
            border: none;
            border-radius: 6px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .btn:hover { transform: translateY(-2px); }
        .link { text-align: center; margin-top: 1.5rem; color: #7f8c8d; }
        .link a { color: #667eea; text-decoration: none; font-weight: 600; }
        .link a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔐 Login</h1>
        <p class="subtitle">Access your account</p>
        )" + msg_html + R"(
        <form method="POST" action="/login">
            <div class="form-group">
                <label>Username</label>
                <input type="text" name="username" required autofocus>
            </div>
            <div class="form-group">
                <label>Password</label>
                <input type="password" name="password" required>
            </div>
            <button type="submit" class="btn">Login</button>
        </form>
        <div class="link">
            Don't have an account? <a href="/signup">Sign up</a>
        </div>
    </div>
</body>
</html>)";
    }

    std::string generate_signup_page(const std::string& message = "") {
        std::string msg_html = message.empty() ? "" : 
            "<div style='background: #e74c3c; color: white; padding: 1rem; border-radius: 4px; margin-bottom: 1rem;'>" + message + "</div>";
        
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>Account Service - Sign Up</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 2rem;
        }
        .container {
            background: white;
            padding: 3rem;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 450px;
            width: 100%;
        }
        h1 { color: #2c3e50; margin-bottom: 0.5rem; font-size: 2rem; }
        .subtitle { color: #7f8c8d; margin-bottom: 2rem; }
        .form-group { margin-bottom: 1.5rem; }
        label { display: block; margin-bottom: 0.5rem; color: #2c3e50; font-weight: 500; }
        input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 1rem;
        }
        input:focus { outline: none; border-color: #667eea; }
        .btn {
            width: 100%;
            padding: 0.875rem;
            border: none;
            border-radius: 6px;
            background: linear-gradient(135deg, #27ae60 0%, #229954 100%);
            color: white;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .btn:hover { transform: translateY(-2px); }
        .link { text-align: center; margin-top: 1.5rem; color: #7f8c8d; }
        .link a { color: #667eea; text-decoration: none; font-weight: 600; }
        .link a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✨ Sign Up</h1>
        <p class="subtitle">Create your account</p>
        )" + msg_html + R"(
        <form method="POST" action="/signup">
            <div class="form-group">
                <label>Username</label>
                <input type="text" name="username" required autofocus>
            </div>
            <div class="form-group">
                <label>Email</label>
                <input type="email" name="email" required>
            </div>
            <div class="form-group">
                <label>Full Name</label>
                <input type="text" name="full_name" required>
            </div>
            <div class="form-group">
                <label>Password</label>
                <input type="password" name="password" required>
            </div>
            <button type="submit" class="btn">Create Account</button>
        </form>
        <div class="link">
            Already have an account? <a href="/">Login</a>
        </div>
    </div>
</body>
</html>)";
    }

    std::string generate_profile_page(const UserAccount& account) {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>Account Service - Profile</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #f5f6fa;
            min-height: 100vh;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 2rem;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .header h1 { font-size: 2rem; margin-bottom: 0.5rem; }
        .container {
            max-width: 800px;
            margin: 2rem auto;
            padding: 0 2rem;
        }
        .card {
            background: white;
            padding: 2rem;
            border-radius: 12px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            margin-bottom: 2rem;
        }
        .card h2 { color: #2c3e50; margin-bottom: 1.5rem; font-size: 1.5rem; }
        .info-grid {
            display: grid;
            gap: 1.5rem;
        }
        .info-item {
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
        }
        .info-label { color: #7f8c8d; font-weight: 600; font-size: 0.9rem; text-transform: uppercase; }
        .info-value { color: #2c3e50; font-size: 1.1rem; }
        .form-group { margin-bottom: 1.5rem; }
        label { display: block; margin-bottom: 0.5rem; color: #2c3e50; font-weight: 500; }
        input, textarea {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 1rem;
            font-family: inherit;
        }
        textarea { min-height: 100px; resize: vertical; }
        input:focus, textarea:focus { outline: none; border-color: #667eea; }
        .btn {
            padding: 0.875rem 2rem;
            border: none;
            border-radius: 6px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .btn-secondary {
            background: #e74c3c;
            color: white;
            margin-left: 1rem;
        }
        .btn:hover { transform: translateY(-2px); }
        .success-msg {
            background: #27ae60;
            color: white;
            padding: 1rem;
            border-radius: 6px;
            margin-bottom: 1rem;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>👤 My Profile</h1>
        <p>Manage your account information</p>
    </div>
    <div class="container">
        <div class="card">
            <h2>Account Information</h2>
            <div class="info-grid">
                <div class="info-item">
                    <div class="info-label">Username</div>
                    <div class="info-value">)" + account.username + R"(</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Email</div>
                    <div class="info-value">)" + account.email + R"(</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Member Since</div>
                    <div class="info-value">)" + account.created_at + R"(</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Last Login</div>
                    <div class="info-value">)" + account.last_login + R"(</div>
                </div>
            </div>
        </div>
        
        <div class="card">
            <h2>Edit Profile</h2>
            <form method="POST" action="/profile/update">
                <div class="form-group">
                    <label>Full Name</label>
                    <input type="text" name="full_name" value=")" + account.full_name + R"(" required>
                </div>
                <div class="form-group">
                    <label>Email</label>
                    <input type="email" name="email" value=")" + account.email + R"(" required>
                </div>
                <div class="form-group">
                    <label>Bio</label>
                    <textarea name="bio">)" + account.bio + R"(</textarea>
                </div>
                <button type="submit" class="btn btn-primary">💾 Save Changes</button>
                <a href="/logout"><button type="button" class="btn btn-secondary">🚪 Logout</button></a>
            </form>
        </div>
    </div>
</body>
</html>)";
    }

    void handle_request(int client_socket) {
        char buffer[8192] = {0};
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        std::string request(buffer);
        std::string session_id = get_session_from_cookie(request);
        
        // Parse request line
        std::istringstream request_stream(request);
        std::string method, path;
        request_stream >> method >> path;

        std::string response;

        // Route handling
        if (path == "/" || path == "/login") {
            if (method == "GET") {
                response = http_response(generate_login_page());
            } else if (method == "POST") {
                size_t body_start = request.find("\r\n\r\n");
                std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";
                
                std::string username = url_decode(parse_post_param(body, "username"));
                std::string password = url_decode(parse_post_param(body, "password"));
                
                std::lock_guard<std::mutex> lock(accounts_mutex_);
                if (accounts_.count(username) && accounts_[username].password == password) {
                    std::string new_session = generate_session_id();
                    sessions_[new_session] = username;
                    accounts_[username].last_login = get_current_time();
                    
                    response = "HTTP/1.1 302 Found\r\n"
                              "Location: /profile\r\n"
                              "Set-Cookie: session_id=" + new_session + "; Path=/\r\n"
                              "Connection: close\r\n\r\n";
                } else {
                    response = http_response(generate_login_page("Invalid username or password"));
                }
            }
        } else if (path == "/signup") {
            if (method == "GET") {
                response = http_response(generate_signup_page());
            } else if (method == "POST") {
                size_t body_start = request.find("\r\n\r\n");
                std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";
                
                std::string username = url_decode(parse_post_param(body, "username"));
                std::string email = url_decode(parse_post_param(body, "email"));
                std::string full_name = url_decode(parse_post_param(body, "full_name"));
                std::string password = url_decode(parse_post_param(body, "password"));
                
                std::lock_guard<std::mutex> lock(accounts_mutex_);
                if (accounts_.count(username)) {
                    response = http_response(generate_signup_page("Username already exists"));
                } else {
                    UserAccount account;
                    account.username = username;
                    account.password = password;
                    account.email = email;
                    account.full_name = full_name;
                    account.created_at = get_current_time();
                    account.last_login = get_current_time();
                    accounts_[username] = account;
                    
                    response = "HTTP/1.1 302 Found\r\n"
                              "Location: /\r\n"
                              "Connection: close\r\n\r\n";
                }
            }
        } else if (path == "/profile") {
            std::lock_guard<std::mutex> lock(accounts_mutex_);
            if (!session_id.empty() && sessions_.count(session_id)) {
                std::string username = sessions_[session_id];
                if (accounts_.count(username)) {
                    response = http_response(generate_profile_page(accounts_[username]));
                } else {
                    response = "HTTP/1.1 302 Found\r\nLocation: /\r\nConnection: close\r\n\r\n";
                }
            } else {
                response = "HTTP/1.1 302 Found\r\nLocation: /\r\nConnection: close\r\n\r\n";
            }
        } else if (path == "/profile/update" && method == "POST") {
            size_t body_start = request.find("\r\n\r\n");
            std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";
            
            std::lock_guard<std::mutex> lock(accounts_mutex_);
            if (!session_id.empty() && sessions_.count(session_id)) {
                std::string username = sessions_[session_id];
                if (accounts_.count(username)) {
                    accounts_[username].full_name = url_decode(parse_post_param(body, "full_name"));
                    accounts_[username].email = url_decode(parse_post_param(body, "email"));
                    accounts_[username].bio = url_decode(parse_post_param(body, "bio"));
                }
            }
            response = "HTTP/1.1 302 Found\r\nLocation: /profile\r\nConnection: close\r\n\r\n";
        } else if (path == "/logout") {
            std::lock_guard<std::mutex> lock(accounts_mutex_);
            if (!session_id.empty()) {
                sessions_.erase(session_id);
            }
            response = "HTTP/1.1 302 Found\r\n"
                      "Location: /\r\n"
                      "Set-Cookie: session_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n"
                      "Connection: close\r\n\r\n";
        } else if (path == "/health" || path == "/api/health") {
            std::string body = "{\"status\":\"ok\"}";
            response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(body.length()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Connection: close\r\n\r\n" + body;
        } else {
            std::string body = "{\"error\": \"Not Found\"}";
            response = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(body.length()) + "\r\n"
                      "Connection: close\r\n\r\n" + body;
        }

        send(client_socket, response.c_str(), response.length(), 0);
        close(client_socket);
    }

public:
    AccountService(int port) : port_(port), server_fd_(-1), running_(false) {}

    ~AccountService() {
        stop();
    }

    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options\n";
            close(server_fd_);
            return false;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port_ << "\n";
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, 10) < 0) {
            std::cerr << "Failed to listen on port " << port_ << "\n";
            close(server_fd_);
            return false;
        }

        running_ = true;
        std::cout << "\n=== Account Service ===" << std::endl;
        std::cout << "✓ Started on port " << port_ << std::endl;
        std::cout << "✓ Open: http://localhost:" << port_ << std::endl;
        std::cout << "✓ Routes:" << std::endl;
        std::cout << "  - GET  /          - Login page" << std::endl;
        std::cout << "  - POST /login     - Login handler" << std::endl;
        std::cout << "  - GET  /signup    - Signup page" << std::endl;
        std::cout << "  - POST /signup    - Signup handler" << std::endl;
        std::cout << "  - GET  /profile   - User profile" << std::endl;
        std::cout << "  - POST /profile/update - Update profile" << std::endl;
        std::cout << "  - GET  /logout    - Logout" << std::endl;
        std::cout << "  - GET  /health    - Health check" << std::endl;
        std::cout << "\nPress Ctrl+C to stop\n" << std::endl;

        return true;
    }

    void run() {
        if (!running_) {
            std::cerr << "Service not started\n";
            return;
        }

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        while (running_) {
            int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "Failed to accept connection\n";
                }
                continue;
            }

            std::cout << "✓ Request from " << inet_ntoa(client_addr.sin_addr) << std::endl;
            handle_request(client_socket);
        }
    }

    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 9002;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port < 1 || port > 65535) {
            std::cerr << "Invalid port number. Using default 9002\n";
            port = 9002;
        }
    }

    srand(time(nullptr));
    AccountService service(port);
    
    if (!service.start()) {
        return 1;
    }

    service.run();
    return 0;
}
