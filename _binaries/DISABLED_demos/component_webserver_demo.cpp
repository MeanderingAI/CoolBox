#include "networking/document/web_components.h"
#include "networking/document/html_processor.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <map>

using namespace ml::networking::html;

class ComponentWebServer {
public:
    ComponentWebServer(int port) : port_(port), running_(false) {}
    
    void add_route(const std::string& path, const std::function<std::string()>& handler) {
        routes_[path] = handler;
    }
    
    void start() {
        running_ = true;
        
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket\n";
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed\n";
            return;
        }
        
        if (listen(server_fd_, 10) < 0) {
            std::cerr << "Listen failed\n";
            return;
        }
        
        std::cout << "✓ Component Web Server running on http://localhost:" << port_ << "\n";
        std::cout << "Available routes:\n";
        for (const auto& [path, _] : routes_) {
            std::cout << "  - http://localhost:" << port_ << path << "\n";
        }
        std::cout << "\nPress Ctrl+C to stop\n\n";
        
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
            
            if (client_fd < 0) continue;
            
            handle_request(client_fd);
            close(client_fd);
        }
    }
    
    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
        }
    }
    
private:
    int port_;
    int server_fd_;
    bool running_;
    std::map<std::string, std::function<std::string()>> routes_;
    
    void handle_request(int client_fd) {
        char buffer[4096] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);
        
        std::string request(buffer);
        size_t path_start = request.find(" ") + 1;
        size_t path_end = request.find(" ", path_start);
        std::string path = request.substr(path_start, path_end - path_start);
        
        // Strip query parameters
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }
        
        std::cout << "→ " << path << "\n";
        
        std::string response;
        std::string content_type = "text/html";
        
        if (routes_.find(path) != routes_.end()) {
            response = routes_[path]();
        } else {
            response = generate_404();
        }
        
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + content_type + "; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response;
        
        write(client_fd, http_response.c_str(), http_response.length());
    }
    
    std::string generate_404() {
        ComponentBundler bundler;
        return bundler
            .set_title("404 Not Found")
            .set_body_content(R"(
                <div style="text-align: center; padding: 4rem;">
                    <h1 style="font-size: 4rem; color: #e74c3c;">404</h1>
                    <h2>Page Not Found</h2>
                    <p>The page you're looking for doesn't exist.</p>
                </div>
            )")
            .bundle();
    }
};

int main() {
    std::cout << "=== Component Web Server Demo ===\n\n";
    
    // Register all components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_app_header());
    registry.register_component(components::create_nav_menu());
    registry.register_component(components::create_card());
    registry.register_component(components::create_button());
    registry.register_component(components::create_form_input());
    registry.register_component(components::create_modal());
    registry.register_component(components::create_toast());
    registry.register_component(components::create_data_table());
    registry.register_component(components::create_progress_bar());
    registry.register_component(components::create_tabs());
    registry.register_component(components::create_dropdown());
    registry.register_component(components::create_accordion());
    registry.register_component(components::create_footer());
    
    std::cout << "✓ Registered " << registry.list_components().size() << " components\n\n";
    
    // Create server
    ComponentWebServer server(8080);
    
    // Route 1: Dashboard
    server.add_route("/", []() {
        ComponentBundler bundler;
        return bundler
            .set_title("ToolBox Dashboard")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1200px; margin: 0 auto; padding: 2rem; }
                .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 2rem; margin: 2rem 0; }
                h1, h2 { color: #2c3e50; margin-bottom: 1rem; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🛠️ ToolBox</span>
                    <nav-menu slot="nav">
                        <a href="/">Home</a>
                        <a href="/components">Components</a>
                        <a href="/demo">Demo</a>
                    </nav-menu>
                </app-header>
                <div class="container">
                    <h1>Welcome to ToolBox</h1>
                    <p>Build modern web applications with reusable components</p>
                    <br>
                    <div class="grid">
                        <app-card>
                            <h3 slot="header">🚀 Fast Development</h3>
                            Pre-built components for rapid prototyping
                        </app-card>
                        <app-card>
                            <h3 slot="header">🎨 Beautiful UI</h3>
                            Modern design with smooth animations
                        </app-card>
                        <app-card>
                            <h3 slot="header">📦 Single File Bundle</h3>
                            All components compiled into one HTML file
                        </app-card>
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .minify(true)
            .bundle();
    });
    
    // Route 2: Components List
    server.add_route("/components", []() {
        ComponentBundler bundler;
        
        std::string cards;
        for (const auto& name : ComponentRegistry::instance().list_components()) {
            cards += "<app-card><h3 slot=\"header\">" + name + "</h3>"
                    "Custom web component<div slot=\"footer\">"
                    "<app-button>View Docs</app-button></div></app-card>";
        }
        
        return bundler
            .set_title("Component Library")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1200px; margin: 0 auto; padding: 2rem; }
                .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1.5rem; margin: 2rem 0; }
                h1 { color: #2c3e50; margin-bottom: 2rem; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🛠️ ToolBox</span>
                    <nav-menu slot="nav">
                        <a href="/">Home</a>
                        <a href="/components">Components</a>
                        <a href="/demo">Demo</a>
                    </nav-menu>
                </app-header>
                <div class="container">
                    <h1>Component Library</h1>
                    <div class="grid">)" + cards + R"(
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .minify(true)
            .bundle();
    });
    
    // Route 3: Interactive Demo
    server.add_route("/demo", []() {
        ComponentBundler bundler;
        return bundler
            .set_title("Interactive Demo")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1200px; margin: 0 auto; padding: 2rem; }
                section { margin: 2rem 0; }
                h1, h2 { color: #2c3e50; margin-bottom: 1rem; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🛠️ ToolBox</span>
                    <nav-menu slot="nav">
                        <a href="/">Home</a>
                        <a href="/components">Components</a>
                        <a href="/demo">Demo</a>
                    </nav-menu>
                </app-header>
                <div class="container">
                    <h1>Interactive Component Demo</h1>
                    
                    <section>
                        <h2>Forms</h2>
                        <app-card>
                            <h3 slot="header">Sign Up</h3>
                            <form-input label="Username" placeholder="Enter username"></form-input>
                            <form-input label="Email" type="email" placeholder="your@email.com"></form-input>
                            <form-input label="Password" type="password"></form-input>
                            <div slot="footer">
                                <app-button>Create Account</app-button>
                            </div>
                        </app-card>
                    </section>
                    
                    <section>
                        <h2>Progress Indicators</h2>
                        <app-card>
                            <div slot="header">Loading Tasks</div>
                            <progress-bar value="85" max="100"></progress-bar>
                            <br>
                            <progress-bar value="60" max="100"></progress-bar>
                            <br>
                            <progress-bar value="30" max="100"></progress-bar>
                        </app-card>
                    </section>
                    
                    <section>
                        <h2>Accordions</h2>
                        <app-accordion>
                            <div slot="header">What is ToolBox?</div>
                            A comprehensive C++ framework for building modern web applications.
                        </app-accordion>
                        <app-accordion>
                            <div slot="header">How to use?</div>
                            Include the headers, create components, and bundle them into HTML.
                        </app-accordion>
                    </section>
                </div>
                <app-footer>
                    <div slot="copyright">© 2025 ToolBox Framework</div>
                </app-footer>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .add_component_from_registry("form-input")
            .add_component_from_registry("progress-bar")
            .add_component_from_registry("app-accordion")
            .add_component_from_registry("app-footer")
            .add_global_script(R"(
                document.addEventListener('DOMContentLoaded', () => {
                    // Animate progress bars
                    document.querySelectorAll('progress-bar').forEach(bar => {
                        const fill = bar.shadowRoot.querySelector('.progress-fill');
                        const text = bar.shadowRoot.querySelector('.progress-text');
                        const value = bar.getAttribute('value') || 0;
                        fill.style.width = value + '%';
                        text.textContent = value + '%';
                    });
                    
                    // Add accordion click handlers
                    document.querySelectorAll('app-accordion').forEach(acc => {
                        const header = acc.shadowRoot.querySelector('.accordion-header');
                        header.addEventListener('click', () => {
                            acc.classList.toggle('open');
                        });
                    });
                });
            )")
            .minify(true)
            .bundle();
    });
    
    // Start server
    std::thread server_thread([&server]() {
        server.start();
    });
    
    // Keep main thread alive
    server_thread.join();
    
    return 0;
}
