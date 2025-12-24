#include "networking/html/web_components.h"
#include "services/mail_server/mail_server.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace ml::networking::html;
using namespace services::mail_server;

class MailWebUI {
public:
    MailWebUI(int http_port, MailServer* mail_server) 
        : http_port_(http_port), mail_server_(mail_server), running_(false) {}
    
    void start() {
        running_ = true;
        
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(http_port_);
        
        bind(server_fd_, (struct sockaddr*)&address, sizeof(address));
        listen(server_fd_, 10);
        
        std::cout << "✓ Mail Web UI running on http://localhost:" << http_port_ << "\n\n";
        
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
    int http_port_;
    int server_fd_;
    bool running_;
    MailServer* mail_server_;
    
    void handle_request(int client_fd) {
        char buffer[4096] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);
        
        std::string request(buffer);
        size_t path_start = request.find(" ") + 1;
        size_t path_end = request.find(" ", path_start);
        std::string path = request.substr(path_start, path_end - path_start);
        
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }
        
        std::string response = generate_mail_ui();
        
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response;
        
        write(client_fd, http_response.c_str(), http_response.length());
    }
    
    std::string generate_mail_ui() {
        ComponentRegistry& registry = ComponentRegistry::instance();
        
        // Create mail-specific components
        WebComponent mail_list = WebComponentBuilder("mail-list")
            .template_html(R"(
                <div class="mail-list">
                    <slot></slot>
                </div>
            )")
            .style(R"(
                .mail-list {
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    overflow: hidden;
                }
            )")
            .build();
        
        WebComponent mail_item = WebComponentBuilder("mail-item")
            .template_html(R"(
                <div class="mail-item">
                    <div class="sender"></div>
                    <div class="subject"></div>
                    <div class="preview"></div>
                    <div class="time"></div>
                </div>
            )")
            .style(R"(
                .mail-item {
                    padding: 1rem;
                    border-bottom: 1px solid #eee;
                    cursor: pointer;
                    display: grid;
                    grid-template-columns: 150px 1fr 100px;
                    gap: 1rem;
                    transition: background 0.2s;
                }
                .mail-item:hover {
                    background: #f9f9f9;
                }
                .sender {
                    font-weight: 600;
                }
                .subject {
                    font-weight: 500;
                }
                .preview {
                    color: #666;
                    font-size: 0.9rem;
                    grid-column: 2;
                }
                .time {
                    color: #999;
                    font-size: 0.85rem;
                    text-align: right;
                }
            )")
            .build();
        
        registry.register_component(mail_list);
        registry.register_component(mail_item);
        
        ComponentBundler bundler;
        return bundler
            .set_title("ToolBox Mail - Web Interface")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body {
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
                    background: #f5f7fa;
                }
                .container {
                    max-width: 1400px;
                    margin: 0 auto;
                    padding: 2rem;
                }
                .mail-layout {
                    display: grid;
                    grid-template-columns: 250px 1fr 400px;
                    gap: 2rem;
                    margin-top: 2rem;
                }
                .sidebar {
                    background: white;
                    padding: 1rem;
                    border-radius: 8px;
                    height: fit-content;
                }
                .sidebar h3 {
                    margin-bottom: 1rem;
                    color: #2c3e50;
                }
                .folder-item {
                    padding: 0.75rem;
                    margin-bottom: 0.5rem;
                    border-radius: 4px;
                    cursor: pointer;
                    transition: background 0.2s;
                }
                .folder-item:hover {
                    background: #f0f0f0;
                }
                .folder-item.active {
                    background: #667eea;
                    color: white;
                }
                .main-content {
                    background: white;
                    border-radius: 8px;
                    padding: 1.5rem;
                }
                .compose-area {
                    background: white;
                    padding: 1.5rem;
                    border-radius: 8px;
                }
                .stats {
                    display: grid;
                    grid-template-columns: repeat(3, 1fr);
                    gap: 1rem;
                    margin-bottom: 2rem;
                }
                .stat-card {
                    background: white;
                    padding: 1.5rem;
                    border-radius: 8px;
                    text-align: center;
                }
                .stat-value {
                    font-size: 2rem;
                    font-weight: bold;
                    color: #667eea;
                }
                .stat-label {
                    color: #666;
                    margin-top: 0.5rem;
                }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">📧 ToolBox Mail</span>
                    <nav-menu slot="nav">
                        <a href="/">Inbox</a>
                        <a href="/sent">Sent</a>
                        <a href="/drafts">Drafts</a>
                        <a href="/settings">Settings</a>
                    </nav-menu>
                    <div slot="actions">
                        <app-button>+ Compose</app-button>
                    </div>
                </app-header>

                <div class="container">
                    <div class="stats">
                        <div class="stat-card">
                            <div class="stat-value">156</div>
                            <div class="stat-label">Total Emails</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-value">23</div>
                            <div class="stat-label">Unread</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-value">5</div>
                            <div class="stat-label">Starred</div>
                        </div>
                    </div>

                    <div class="mail-layout">
                        <div class="sidebar">
                            <h3>Folders</h3>
                            <div class="folder-item active">
                                📥 Inbox (23)
                            </div>
                            <div class="folder-item">
                                📤 Sent
                            </div>
                            <div class="folder-item">
                                📝 Drafts (2)
                            </div>
                            <div class="folder-item">
                                ⭐ Starred
                            </div>
                            <div class="folder-item">
                                🗑️ Trash
                            </div>
                        </div>

                        <div class="main-content">
                            <h2>Inbox</h2>
                            <mail-list>
                                <mail-item>
                                    <div class="sender">John Doe</div>
                                    <div class="subject">Project Update - Q4 2025</div>
                                    <div class="preview">Here's the latest update on our project progress...</div>
                                    <div class="time">2 hours ago</div>
                                </mail-item>
                                <mail-item>
                                    <div class="sender">Sarah Smith</div>
                                    <div class="subject">Meeting Tomorrow</div>
                                    <div class="preview">Just a reminder about our meeting scheduled for tomorrow at 10 AM...</div>
                                    <div class="time">5 hours ago</div>
                                </mail-item>
                                <mail-item>
                                    <div class="sender">ToolBox Team</div>
                                    <div class="subject">New Features Available</div>
                                    <div class="preview">We're excited to announce new features in ToolBox v2.0...</div>
                                    <div class="time">Yesterday</div>
                                </mail-item>
                                <mail-item>
                                    <div class="sender">Mike Johnson</div>
                                    <div class="subject">Code Review Request</div>
                                    <div class="preview">Could you please review my latest pull request...</div>
                                    <div class="time">2 days ago</div>
                                </mail-item>
                            </mail-list>
                        </div>

                        <div class="compose-area">
                            <h3>Compose Email</h3>
                            <app-card>
                                <form-input label="To" placeholder="recipient@example.com"></form-input>
                                <form-input label="Subject" placeholder="Email subject"></form-input>
                                <form-input label="Message" placeholder="Write your message..."></form-input>
                                <div slot="footer">
                                    <app-button>Send Email</app-button>
                                </div>
                            </app-card>
                            
                            <br>
                            
                            <app-accordion>
                                <div slot="header">📊 Server Stats</div>
                                <div>
                                    <p><strong>SMTP Status:</strong> Active</p>
                                    <p><strong>POP3 Status:</strong> Active</p>
                                    <p><strong>Emails Sent:</strong> 1,234</p>
                                    <p><strong>Emails Received:</strong> 2,567</p>
                                    <p><strong>Uptime:</strong> 15 days</p>
                                </div>
                            </app-accordion>
                        </div>
                    </div>
                </div>

                <app-footer>
                    <div slot="center">
                        <p>ToolBox Mail Server - Powered by C++</p>
                    </div>
                    <div slot="copyright">© 2025 ToolBox Framework</div>
                </app-footer>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-button")
            .add_component_from_registry("app-card")
            .add_component_from_registry("form-input")
            .add_component_from_registry("app-accordion")
            .add_component_from_registry("app-footer")
            .add_component_from_registry("mail-list")
            .add_component_from_registry("mail-item")
            .add_global_script(R"(
                document.addEventListener('DOMContentLoaded', () => {
                    // Add mail item click handlers
                    document.querySelectorAll('mail-item').forEach(item => {
                        item.addEventListener('click', () => {
                            console.log('Email clicked:', item);
                            alert('Email viewer would open here!');
                        });
                    });
                    
                    // Add folder click handlers
                    document.querySelectorAll('.folder-item').forEach(folder => {
                        folder.addEventListener('click', () => {
                            document.querySelectorAll('.folder-item').forEach(f => f.classList.remove('active'));
                            folder.classList.add('active');
                        });
                    });
                    
                    // Add accordion handlers
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
    }
};

int main() {
    std::cout << "=== Mail Server Web UI Demo ===\n\n";
    
    // Register components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_app_header());
    registry.register_component(components::create_nav_menu());
    registry.register_component(components::create_card());
    registry.register_component(components::create_button());
    registry.register_component(components::create_form_input());
    registry.register_component(components::create_accordion());
    registry.register_component(components::create_footer());
    
    // Create mail server
    MailServer mail_server(2525, 11110);
    
    std::cout << "✓ Starting Mail Server...\n";
    std::thread mail_thread([&mail_server]() {
        mail_server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Create web UI
    std::cout << "✓ Starting Web UI...\n";
    MailWebUI web_ui(8081, &mail_server);
    
    std::thread ui_thread([&web_ui]() {
        web_ui.start();
    });
    
    std::cout << "\nMail Server Status:\n";
    std::cout << "  SMTP Port: 2525\n";
    std::cout << "  POP3 Port: 11110\n";
    std::cout << "  Web UI: http://localhost:8081\n";
    std::cout << "\nOpen http://localhost:8081 in your browser!\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    ui_thread.join();
    mail_thread.join();
    
    return 0;
}
