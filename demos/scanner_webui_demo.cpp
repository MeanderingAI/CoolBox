#include "networking/html/web_components.h"
#include "system/host_scanner.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

using namespace ml::networking::html;
using namespace host::scanner;

class ScannerWebUI {
public:
    ScannerWebUI(int port, HostScanner* scanner) 
        : port_(port), scanner_(scanner), running_(false) {}
    
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
        
        std::cout << "✓ Scanner Web UI running on http://localhost:" << port_ << "\n\n";
        
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
    HostScanner* scanner_;
    
    void handle_request(int client_fd) {
        char buffer[4096] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);
        
        std::string request(buffer);
        size_t path_start = request.find(" ") + 1;
        size_t path_end = request.find(" ", path_start);
        std::string path = request.substr(path_start, path_end - path_start);
        
        std::string response;
        if (path == "/" || path == "/dashboard") {
            response = generate_dashboard();
        } else if (path == "/system") {
            response = generate_system_page();
        } else if (path == "/network") {
            response = generate_network_page();
        } else if (path == "/scan") {
            response = generate_scan_page();
        } else {
            response = generate_dashboard();
        }
        
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response;
        
        write(client_fd, http_response.c_str(), http_response.length());
    }
    
    std::string generate_dashboard() {
        auto report = scanner_->generate_report();
        ComponentBundler bundler;
        
        return bundler
            .set_title("System & Network Scanner - Dashboard")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
                .container { max-width: 1600px; margin: 0 auto; padding: 2rem; }
                .hero {
                    background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
                    padding: 3rem;
                    border-radius: 12px;
                    margin-bottom: 2rem;
                }
                .hero h1 { font-size: 2.5rem; margin-bottom: 0.5rem; }
                .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1.5rem; margin: 2rem 0; }
                .stat-card { background: #1e293b; padding: 1.5rem; border-radius: 8px; border-left: 4px solid #3b82f6; }
                .stat-value { font-size: 2.5rem; font-weight: bold; color: #3b82f6; margin: 0.5rem 0; }
                .stat-label { color: #94a3b8; font-size: 0.9rem; }
                .grid-2 { display: grid; grid-template-columns: repeat(2, 1fr); gap: 2rem; margin: 2rem 0; }
                @media (max-width: 768px) { .grid-2 { grid-template-columns: 1fr; } }
                .card { background: #1e293b; padding: 2rem; border-radius: 8px; margin-bottom: 1.5rem; }
                .card h3 { color: #3b82f6; margin-bottom: 1rem; }
                .metric { display: flex; justify-content: space-between; padding: 0.75rem 0; border-bottom: 1px solid #334155; }
                .metric:last-child { border-bottom: none; }
                .badge { background: #3b82f6; color: white; padding: 0.25rem 0.75rem; border-radius: 12px; font-size: 0.85rem; }
            )")
            .set_body_content(R"(
                <app-header style="background: #1e293b;">
                    <span slot="logo">🖥️ System Scanner</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard" style="color: #e2e8f0;">Dashboard</a>
                        <a href="/system" style="color: #e2e8f0;">System</a>
                        <a href="/network" style="color: #e2e8f0;">Network</a>
                        <a href="/scan" style="color: #e2e8f0;">Scan</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <div class="hero">
                        <h1>📊 Host Scanner Dashboard</h1>
                        <p>Real-time system and network monitoring • )" + report.hostname + R"(</p>
                    </div>

                    <div class="stats-grid">
                        <div class="stat-card">
                            <div class="stat-label">CPU Usage</div>
                            <div class="stat-value">)" + std::to_string(static_cast<int>(report.cpu.usage_percent)) + R"(%</div>
                            <progress-bar value=")" + std::to_string(static_cast<int>(report.cpu.usage_percent)) + R"(" max="100"></progress-bar>
                        </div>
                        <div class="stat-card">
                            <div class="stat-label">Memory Usage</div>
                            <div class="stat-value">)" + std::to_string(static_cast<int>(report.memory.usage_percent)) + R"(%</div>
                            <progress-bar value=")" + std::to_string(static_cast<int>(report.memory.usage_percent)) + R"(" max="100"></progress-bar>
                        </div>
                        <div class="stat-card">
                            <div class="stat-label">Network Interfaces</div>
                            <div class="stat-value">)" + std::to_string(report.interfaces.size()) + R"(</div>
                            <div class="stat-label">Active interfaces</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-label">Total Disk</div>
                            <div class="stat-value">)" + (report.disks.empty() ? "0 GB" : format_bytes(report.disks[0].total_bytes)) + R"(</div>
                            <div class="stat-label">Primary disk</div>
                        </div>
                    </div>

                    <div class="grid-2">
                        <div class="card">
                            <h3>💻 System Information</h3>
                            <div class="metric">
                                <span>Hostname</span>
                                <span class="badge">)" + report.hostname + R"(</span>
                            </div>
                            <div class="metric">
                                <span>Operating System</span>
                                <span>)" + report.os_info + R"(</span>
                            </div>
                            <div class="metric">
                                <span>CPU Model</span>
                                <span>)" + report.cpu.model.substr(0, 30) + R"(</span>
                            </div>
                            <div class="metric">
                                <span>CPU Cores</span>
                                <span>)" + std::to_string(report.cpu.cores) + " physical / " + std::to_string(report.cpu.threads) + R"( logical</span>
                            </div>
                            <div class="metric">
                                <span>Total Memory</span>
                                <span>)" + format_bytes(report.memory.total_bytes) + R"(</span>
                            </div>
                        </div>

                        <div class="card">
                            <h3>🌐 Network Summary</h3>
                            <div class="metric">
                                <span>Active Interfaces</span>
                                <span class="badge">)" + std::to_string(report.interfaces.size()) + R"(</span>
                            </div>
                            )" + [&]() {
                                std::stringstream ss;
                                for (const auto& iface : report.interfaces) {
                                    ss << R"(<div class="metric">)";
                                    ss << "<span>" << iface.name << "</span>";
                                    ss << "<span>" << (iface.ip_addresses.empty() ? "No IP" : iface.ip_addresses[0]) << "</span>";
                                    ss << "</div>";
                                }
                                return ss.str();
                            }() + R"(
                        </div>
                    </div>

                    <div class="card">
                        <h3>💾 Disk Usage</h3>
                        )" + [&]() {
                            std::stringstream ss;
                            for (const auto& disk : report.disks) {
                                ss << R"(<div class="metric">)";
                                ss << "<span>" << disk.mount_point << " (" << disk.filesystem << ")</span>";
                                ss << "<span>" << format_bytes(disk.used_bytes) << " / " << format_bytes(disk.total_bytes);
                                ss << " (" << std::fixed << std::setprecision(1) << disk.usage_percent << "%)</span>";
                                ss << "</div>";
                                ss << R"(<progress-bar value=")" << static_cast<int>(disk.usage_percent) << R"(" max="100"></progress-bar><br>)";
                            }
                            return ss.str();
                        }() + R"(
                    </div>
                </div>

                <script>
                    // Auto-refresh every 5 seconds
                    setTimeout(() => location.reload(), 5000);
                    
                    // Animate progress bars
                    document.querySelectorAll('progress-bar').forEach(bar => {
                        const value = bar.getAttribute('value');
                        const fill = bar.shadowRoot.querySelector('.progress-fill');
                        const text = bar.shadowRoot.querySelector('.progress-text');
                        fill.style.width = value + '%';
                        text.textContent = value + '%';
                        
                        // Color coding
                        if (value > 80) fill.style.background = '#ef4444';
                        else if (value > 60) fill.style.background = '#f59e0b';
                        else fill.style.background = '#10b981';
                    });
                </script>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("progress-bar")
            .minify(true)
            .bundle();
    }
    
    std::string generate_system_page() {
        auto report = scanner_->generate_report();
        ComponentBundler bundler;
        
        return bundler
            .set_title("System Resources")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
                .container { max-width: 1600px; margin: 0 auto; padding: 2rem; }
                .card { background: #1e293b; padding: 2rem; border-radius: 8px; margin-bottom: 1.5rem; }
                .card h2 { color: #3b82f6; margin-bottom: 1rem; }
                table { width: 100%; border-collapse: collapse; }
                th, td { padding: 1rem; text-align: left; border-bottom: 1px solid #334155; }
                th { color: #3b82f6; }
            )")
            .set_body_content(R"(
                <app-header style="background: #1e293b;">
                    <span slot="logo">🖥️ System Scanner</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard" style="color: #e2e8f0;">Dashboard</a>
                        <a href="/system" style="color: #e2e8f0;">System</a>
                        <a href="/network" style="color: #e2e8f0;">Network</a>
                        <a href="/scan" style="color: #e2e8f0;">Scan</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>💻 System Resources</h1>
                    
                    <div class="card">
                        <h2>Top Processes</h2>
                        <table>
                            <tr><th>PID</th><th>Name</th><th>User</th><th>State</th></tr>
                            )" + [&]() {
                                std::stringstream ss;
                                for (const auto& proc : report.top_processes) {
                                    ss << "<tr>";
                                    ss << "<td>" << proc.pid << "</td>";
                                    ss << "<td>" << proc.name << "</td>";
                                    ss << "<td>" << proc.user << "</td>";
                                    ss << "<td>" << proc.state << "</td>";
                                    ss << "</tr>";
                                }
                                return ss.str();
                            }() + R"(
                        </table>
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .minify(true)
            .bundle();
    }
    
    std::string generate_network_page() {
        auto interfaces = scanner_->network_scanner().get_network_interfaces();
        ComponentBundler bundler;
        
        return bundler
            .set_title("Network Interfaces")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
                .container { max-width: 1600px; margin: 0 auto; padding: 2rem; }
                .card { background: #1e293b; padding: 2rem; border-radius: 8px; margin-bottom: 1.5rem; }
                .badge { background: #10b981; color: white; padding: 0.25rem 0.75rem; border-radius: 12px; font-size: 0.85rem; }
                .metric { display: flex; justify-content: space-between; padding: 0.75rem 0; border-bottom: 1px solid #334155; }
            )")
            .set_body_content(R"(
                <app-header style="background: #1e293b;">
                    <span slot="logo">🖥️ System Scanner</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard" style="color: #e2e8f0;">Dashboard</a>
                        <a href="/system" style="color: #e2e8f0;">System</a>
                        <a href="/network" style="color: #e2e8f0;">Network</a>
                        <a href="/scan" style="color: #e2e8f0;">Scan</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>🌐 Network Interfaces</h1>
                    
                    )" + [&]() {
                        std::stringstream ss;
                        for (const auto& iface : interfaces) {
                            ss << R"(<div class="card">)";
                            ss << "<h2>" << iface.name << " <span class='badge'>" << iface.status << "</span></h2>";
                            ss << R"(<div class="metric"><span>MAC Address</span><span>)" << iface.mac_address << "</span></div>";
                            for (const auto& ip : iface.ip_addresses) {
                                ss << R"(<div class="metric"><span>IP Address</span><span>)" << ip << "</span></div>";
                            }
                            ss << R"(<div class="metric"><span>Bytes Sent</span><span>)" << format_bytes(iface.bytes_sent) << "</span></div>";
                            ss << R"(<div class="metric"><span>Bytes Received</span><span>)" << format_bytes(iface.bytes_received) << "</span></div>";
                            ss << "</div>";
                        }
                        return ss.str();
                    }() + R"(
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .minify(true)
            .bundle();
    }
    
    std::string generate_scan_page() {
        ComponentBundler bundler;
        
        return bundler
            .set_title("Port Scanner")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #0f172a; color: #e2e8f0; }
                .container { max-width: 1600px; margin: 0 auto; padding: 2rem; }
                .card { background: #1e293b; padding: 2rem; border-radius: 8px; margin-bottom: 1.5rem; }
                input, button { padding: 0.75rem; border-radius: 4px; border: 1px solid #334155; background: #0f172a; color: #e2e8f0; }
                button { background: #3b82f6; cursor: pointer; margin-left: 0.5rem; }
                button:hover { background: #2563eb; }
            )")
            .set_body_content(R"(
                <app-header style="background: #1e293b;">
                    <span slot="logo">🖥️ System Scanner</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard" style="color: #e2e8f0;">Dashboard</a>
                        <a href="/system" style="color: #e2e8f0;">System</a>
                        <a href="/network" style="color: #e2e8f0;">Network</a>
                        <a href="/scan" style="color: #e2e8f0;">Scan</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>🔍 Network Scanner</h1>
                    
                    <div class="card">
                        <h2>Port Scanner</h2>
                        <p>Scan ports on a target host</p>
                        <br>
                        <input type="text" placeholder="Target IP" style="width: 300px;">
                        <button>Scan Common Ports</button>
                    </div>
                    
                    <div class="card">
                        <h2>Network Discovery</h2>
                        <p>Discover devices on your local network</p>
                        <br>
                        <button>Discover Devices</button>
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .minify(true)
            .bundle();
    }
};

int main() {
    std::cout << "=== System & Network Scanner Web UI ===\n\n";
    
    // Register components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_app_header());
    registry.register_component(components::create_nav_menu());
    registry.register_component(components::create_progress_bar());
    
    // Create scanner
    HostScanner scanner;
    
    // Generate initial report
    std::cout << "✓ Scanning system...\n";
    auto report = scanner.generate_report();
    
    std::cout << "\n📊 System Information:\n";
    std::cout << "  Hostname: " << report.hostname << "\n";
    std::cout << "  OS: " << report.os_info << "\n";
    std::cout << "  CPU: " << report.cpu.model.substr(0, 50) << "\n";
    std::cout << "  CPU Usage: " << std::fixed << std::setprecision(1) << report.cpu.usage_percent << "%\n";
    std::cout << "  Memory Usage: " << report.memory.usage_percent << "%\n";
    std::cout << "  Network Interfaces: " << report.interfaces.size() << "\n";
    
    // Start web UI
    std::cout << "\n✓ Starting web interface...\n";
    ScannerWebUI web_ui(8083, &scanner);
    
    std::cout << "\n🌐 Available at http://localhost:8083\n";
    std::cout << "\nPages:\n";
    std::cout << "  - http://localhost:8083/dashboard (Overview)\n";
    std::cout << "  - http://localhost:8083/system (System Resources)\n";
    std::cout << "  - http://localhost:8083/network (Network Interfaces)\n";
    std::cout << "  - http://localhost:8083/scan (Port Scanner)\n";
    std::cout << "\nPress Ctrl+C to stop\n\n";
    
    web_ui.start();
    
    return 0;
}
