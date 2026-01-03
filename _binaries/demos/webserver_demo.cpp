#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "networking/html/html_processor.h"

using namespace ml::networking::html;

// Simple HTTP Server
class HttpServer {
public:
    HttpServer(int port) : port_(port), running_(false), server_socket_(-1) {}
    
    ~HttpServer() {
        stop();
    }
    
    bool start() {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }
        
        // Allow port reuse
        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);
        
        if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind to port " << port_ << "\n";
            close(server_socket_);
            return false;
        }
        
        if (listen(server_socket_, 5) < 0) {
            std::cerr << "Failed to listen\n";
            close(server_socket_);
            return false;
        }
        
        running_ = true;
        std::cout << "✓ HTTP Server started on http://localhost:" << port_ << "\n";
        
        return true;
    }
    
    void stop() {
        running_ = false;
        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }
    }
    
    void run() {
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "Failed to accept connection\n";
                }
                continue;
            }
            
            handle_client(client_socket);
            close(client_socket);
        }
    }
    
    void set_route_handler(const std::string& path, std::function<std::string()> handler) {
        routes_[path] = handler;
    }
    
private:
    int port_;
    bool running_;
    int server_socket_;
    std::map<std::string, std::function<std::string()>> routes_;
    
    void handle_client(int client_socket) {
        char buffer[4096];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            return;
        }
        
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        
        // Parse request
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;
        
        // Remove query parameters from path
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }
        
        std::cout << "→ " << method << " " << path << "\n";
        
        // Handle routes
        std::string response;
        std::string content_type = "text/html";
        
        if (path == "/style.css" || path == "/styles.css") {
            response = get_stylesheet();
            content_type = "text/css";
        } else if (routes_.find(path) != routes_.end()) {
            response = routes_[path]();
        } else {
            response = generate_404();
        }
        
        send_response(client_socket, "200 OK", content_type, response);
    }
    
    void send_response(int client_socket, const std::string& status, 
                      const std::string& content_type, const std::string& body) {
        std::ostringstream response;
        response << "HTTP/1.1 " << status << "\r\n";
        response << "Content-Type: " << content_type << "; charset=utf-8\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        
        std::string response_str = response.str();
        send(client_socket, response_str.c_str(), response_str.length(), 0);
    }
    
    std::string get_stylesheet() {
        return R"(/* Modern Website Stylesheet */
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: #f8f9fa; }
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 1rem 0; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
header .container { display: flex; justify-content: space-between; align-items: center; }
.logo { font-size: 1.5rem; font-weight: bold; }
nav ul { display: flex; list-style: none; gap: 2rem; }
nav a { color: white; text-decoration: none; transition: opacity 0.3s; }
nav a:hover { opacity: 0.8; }
.hero { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 6rem 0; text-align: center; }
.hero h1 { font-size: 3rem; margin-bottom: 1rem; }
.hero p { font-size: 1.25rem; margin-bottom: 2rem; opacity: 0.9; }
.btn { display: inline-block; padding: 0.75rem 2rem; background: white; color: #667eea; text-decoration: none; border-radius: 5px; font-weight: bold; transition: transform 0.3s; }
.btn:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }
.features { padding: 4rem 0; background: white; }
.features h2 { text-align: center; font-size: 2.5rem; margin-bottom: 3rem; color: #667eea; }
.feature-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 2rem; }
.feature-card { padding: 2rem; background: #f8f9fa; border-radius: 10px; text-align: center; transition: transform 0.3s; }
.feature-card:hover { transform: translateY(-5px); box-shadow: 0 5px 20px rgba(0,0,0,0.1); }
.feature-icon { font-size: 3rem; margin-bottom: 1rem; }
.feature-card h3 { color: #667eea; margin-bottom: 1rem; }
.about { padding: 4rem 0; background: #f8f9fa; }
.about h2 { font-size: 2.5rem; margin-bottom: 2rem; color: #667eea; }
.about-content { display: grid; grid-template-columns: 1fr 1fr; gap: 3rem; align-items: center; }
.about-text p { margin-bottom: 1rem; font-size: 1.1rem; }
.about-image { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); height: 300px; border-radius: 10px; display: flex; align-items: center; justify-content: center; color: white; font-size: 2rem; }
.stats { padding: 4rem 0; background: white; text-align: center; }
.stats h2 { font-size: 2.5rem; margin-bottom: 3rem; color: #667eea; }
.stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 2rem; }
.stat-card { padding: 2rem; }
.stat-number { font-size: 3rem; font-weight: bold; color: #667eea; }
.stat-label { font-size: 1.1rem; color: #666; margin-top: 0.5rem; }
footer { background: #2d3748; color: white; padding: 3rem 0 1rem; }
.footer-content { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 2rem; margin-bottom: 2rem; }
.footer-section h3 { margin-bottom: 1rem; color: #667eea; }
.footer-section ul { list-style: none; }
.footer-section li { margin-bottom: 0.5rem; }
.footer-section a { color: #cbd5e0; text-decoration: none; transition: color 0.3s; }
.footer-section a:hover { color: white; }
.footer-bottom { text-align: center; padding-top: 2rem; border-top: 1px solid #4a5568; color: #cbd5e0; }
@media (max-width: 768px) { .hero h1 { font-size: 2rem; } .about-content { grid-template-columns: 1fr; } nav ul { flex-direction: column; gap: 1rem; } })";
    }
    
    std::string generate_404() {
        HtmlDocument doc;
        doc.set_title("404 - Not Found");
        
        auto body = doc.get_body();
        if (body) {
            auto container = HtmlBuilder("div")
                .class_name("container")
                .style("text-align", "center")
                .style("padding", "4rem 0")
                .child("h1", [](HtmlBuilder& h1) {
                    h1.text("404 - Page Not Found");
                })
                .child("p", [](HtmlBuilder& p) {
                    p.text("The page you're looking for doesn't exist.");
                })
                .child("a", [](HtmlBuilder& a) {
                    a.attr("href", "/").class_name("btn").text("Go Home");
                })
                .build();
            
            body->add_element(std::move(container));
        }
        
        return doc.to_string();
    }
};

// Build the complete website
void build_website_html(HtmlDocument& doc) {
    doc.set_title("TechCorp - Modern Web Solutions");
    doc.add_meta("charset", "UTF-8");
    doc.add_meta("viewport", "width=device-width, initial-scale=1.0");
    doc.add_stylesheet("style.css");
    
    auto body = doc.get_body();
    if (!body) return;
    
    // Header
    auto header = HtmlBuilder("header")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("div", [](HtmlBuilder& logo) {
                    logo.class_name("logo").text("🚀 TechCorp");
                })
                .child("nav", [](HtmlBuilder& nav) {
                    nav.child("ul", [](HtmlBuilder& ul) {
                        ul.child("li", [](HtmlBuilder& li) {
                            li.child("a", [](HtmlBuilder& a) { a.attr("href", "#home").text("Home"); });
                        })
                        .child("li", [](HtmlBuilder& li) {
                            li.child("a", [](HtmlBuilder& a) { a.attr("href", "#features").text("Features"); });
                        })
                        .child("li", [](HtmlBuilder& li) {
                            li.child("a", [](HtmlBuilder& a) { a.attr("href", "#about").text("About"); });
                        })
                        .child("li", [](HtmlBuilder& li) {
                            li.child("a", [](HtmlBuilder& a) { a.attr("href", "#stats").text("Stats"); });
                        });
                    });
                });
        })
        .build();
    body->add_element(std::move(header));
    
    // Hero
    auto hero = HtmlBuilder("section")
        .class_name("hero")
        .id("home")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h1", [](HtmlBuilder& h1) { h1.text("Welcome to the Future"); })
                .child("p", [](HtmlBuilder& p) { p.text("Powered by C++ HTML Processing Library"); })
                .child("a", [](HtmlBuilder& a) { a.class_name("btn").attr("href", "#features").text("Explore"); });
        })
        .build();
    body->add_element(std::move(hero));
    
    // Features
    auto features = HtmlBuilder("section")
        .class_name("features")
        .id("features")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) { h2.text("Powerful Features"); })
                .child("div", [](HtmlBuilder& grid) {
                    grid.class_name("feature-grid")
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("feature-card")
                                .child("div", [](HtmlBuilder& icon) { icon.class_name("feature-icon").text("⚡"); })
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Lightning Fast"); })
                                .child("p", [](HtmlBuilder& p) { p.text("Built with C++17 for maximum performance"); });
                        })
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("feature-card")
                                .child("div", [](HtmlBuilder& icon) { icon.class_name("feature-icon").text("🛡️"); })
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Type Safe"); })
                                .child("p", [](HtmlBuilder& p) { p.text("Compile-time safety and runtime efficiency"); });
                        })
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("feature-card")
                                .child("div", [](HtmlBuilder& icon) { icon.class_name("feature-icon").text("🎨"); })
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Fluent API"); })
                                .child("p", [](HtmlBuilder& p) { p.text("Elegant builder pattern for HTML construction"); });
                        });
                });
        })
        .build();
    body->add_element(std::move(features));
    
    // About
    auto about = HtmlBuilder("section")
        .class_name("about")
        .id("about")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) { h2.text("About Our Technology"); })
                .child("div", [](HtmlBuilder& content) {
                    content.class_name("about-content")
                        .child("div", [](HtmlBuilder& text) {
                            text.class_name("about-text")
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("This website is generated and served entirely using our C++ HTML processing library and custom HTTP server.");
                                })
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("The HTML is built programmatically using a fluent API, demonstrating the power of compile-time type safety combined with runtime flexibility.");
                                })
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("Perfect for web services, API responses, and dynamic content generation.");
                                });
                        })
                        .child("div", [](HtmlBuilder& image) {
                            image.class_name("about-image").text("🌐");
                        });
                });
        })
        .build();
    body->add_element(std::move(about));
    
    // Stats
    auto stats = HtmlBuilder("section")
        .class_name("stats")
        .id("stats")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) { h2.text("Live Statistics"); })
                .child("div", [](HtmlBuilder& grid) {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    
                    grid.class_name("stats-grid")
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("stat-card")
                                .child("div", [](HtmlBuilder& num) { num.class_name("stat-number").text("100%"); })
                                .child("div", [](HtmlBuilder& label) { label.class_name("stat-label").text("C++ Native"); });
                        })
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("stat-card")
                                .child("div", [](HtmlBuilder& num) { num.class_name("stat-number").text("< 1ms"); })
                                .child("div", [](HtmlBuilder& label) { label.class_name("stat-label").text("Response Time"); });
                        })
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("stat-card")
                                .child("div", [](HtmlBuilder& num) { num.class_name("stat-number").text("0"); })
                                .child("div", [](HtmlBuilder& label) { label.class_name("stat-label").text("Dependencies"); });
                        })
                        .child("div", [](HtmlBuilder& card) {
                            card.class_name("stat-card")
                                .child("div", [](HtmlBuilder& num) { num.class_name("stat-number").text("∞"); })
                                .child("div", [](HtmlBuilder& label) { label.class_name("stat-label").text("Possibilities"); });
                        });
                });
        })
        .build();
    body->add_element(std::move(stats));
    
    // Footer
    auto footer = HtmlBuilder("footer")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("div", [](HtmlBuilder& content) {
                    content.class_name("footer-content")
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Technology"); })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) { li.text("C++17"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("HTML Processor"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("HTTP Server"); });
                                });
                        })
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Features"); })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) { li.text("Fluent API"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("Type Safe"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("Fast"); });
                                });
                        })
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) { h3.text("Info"); })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) { li.text("Real-time Generation"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("Zero Templates"); })
                                      .child("li", [](HtmlBuilder& li) { li.text("Pure Code"); });
                                });
                        });
                })
                .child("div", [](HtmlBuilder& bottom) {
                    bottom.class_name("footer-bottom")
                        .child("p", [](HtmlBuilder& p) {
                            p.text("© 2025 TechCorp. Generated with HTML Processor Library.");
                        });
                });
        })
        .build();
    body->add_element(std::move(footer));
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       HTTP Server with HTML Generation            ║\n";
    std::cout << "║       Serving Dynamic Content                      ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    HttpServer server(8080);
    
    // Set up route handler that generates HTML on-the-fly
    server.set_route_handler("/", []() {
        HtmlDocument doc;
        build_website_html(doc);
        return doc.to_string();
    });
    
    if (!server.start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }
    
    std::cout << "\n";
    std::cout << "Server Information:\n";
    std::cout << "  URL: http://localhost:8080\n";
    std::cout << "  Status: Running\n";
    std::cout << "  Content: Dynamically generated HTML\n\n";
    
    std::cout << "Features:\n";
    std::cout << "  ✓ HTML generated on-the-fly per request\n";
    std::cout << "  ✓ CSS served from memory\n";
    std::cout << "  ✓ No file system required\n";
    std::cout << "  ✓ Pure C++ implementation\n\n";
    
    std::cout << "Press Ctrl+C to stop the server\n\n";
    std::cout << "Request Log:\n";
    std::cout << "─────────────────────────────────────────\n";
    
    server.run();
    
    return 0;
}
