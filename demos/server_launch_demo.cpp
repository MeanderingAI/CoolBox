#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "services/server_launch/service_manager.h"
#include "services/cache_server/cache_server.h"
#include "services/dns/dns_server.h"
#include "services/proxy/proxy_server.h"

using namespace services;
using namespace services::server_launch;

// Adapter for CacheServer
class CacheServiceAdapter : public IService {
public:
    CacheServiceAdapter(int port) : server_(port) {}
    
    bool start() override {
        return server_.start();
    }
    
    void stop() override {
        server_.stop();
    }
    
    bool is_running() const override {
        return server_.is_running();
    }
    
    std::string get_name() const override {
        return "CacheServer";
    }
    
    std::string get_status_string() const override {
        return is_running() ? "Running" : "Stopped";
    }
    
private:
    CacheServer server_;
};

// Adapter for DNSServer
class DNSServiceAdapter : public IService {
public:
    DNSServiceAdapter(int port) : server_(port) {}
    
    bool start() override {
        return server_.start();
    }
    
    void stop() override {
        server_.stop();
    }
    
    bool is_running() const override {
        return server_.is_running();
    }
    
    std::string get_name() const override {
        return "DNSServer";
    }
    
    std::string get_status_string() const override {
        return is_running() ? "Running" : "Stopped";
    }
    
private:
    dns::DNSServer server_;
};

// Adapter for ProxyServer
class ProxyServiceAdapter : public IService {
public:
    ProxyServiceAdapter(int port) {
        proxy::ProxyConfig config;
        config.port = port;
        server_ = std::make_unique<proxy::ProxyServer>(config);
    }
    
    bool start() override {
        return server_->start();
    }
    
    void stop() override {
        server_->stop();
    }
    
    bool is_running() const override {
        return server_->is_running();
    }
    
    std::string get_name() const override {
        return "ProxyServer";
    }
    
    std::string get_status_string() const override {
        return is_running() ? "Running" : "Stopped";
    }
    
private:
    std::unique_ptr<proxy::ProxyServer> server_;
};

void print_service_status(const ServiceManager& manager) {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║              Service Status                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    auto statuses = manager.get_all_statuses();
    
    std::cout << "Service Name          | Status\n";
    std::cout << "----------------------+------------------\n";
    
    for (const auto& [name, status] : statuses) {
        std::cout << std::setw(20) << std::left << name << " | " 
                  << status_to_string(status) << "\n";
    }
    
    std::cout << "\nRunning: " << manager.get_running_count() 
              << " / " << manager.get_total_count() << "\n";
}

void demo_service_registration() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Service Registration Demo           ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ServiceManager manager;
    
    // Register services
    std::cout << "Registering services...\n";
    manager.register_service("cache", std::make_unique<CacheServiceAdapter>(6379));
    manager.register_service("dns", std::make_unique<DNSServiceAdapter>(53));
    manager.register_service("proxy", std::make_unique<ProxyServiceAdapter>(8080));
    
    std::cout << "Registered " << manager.get_total_count() << " services\n";
    
    auto names = manager.get_service_names();
    for (const auto& name : names) {
        std::cout << "  - " << name << "\n";
    }
}

void demo_service_lifecycle() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Service Lifecycle Demo              ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ServiceManager manager;
    
    // Register services
    manager.register_service("cache", std::make_unique<CacheServiceAdapter>(6379));
    manager.register_service("dns", std::make_unique<DNSServiceAdapter>(53));
    
    print_service_status(manager);
    
    // Start individual service
    std::cout << "\nStarting cache service...\n";
    if (manager.start_service("cache")) {
        std::cout << "Cache service started successfully\n";
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
    
    // Start all services
    std::cout << "\nStarting all services...\n";
    manager.start_all();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
    
    // Stop individual service
    std::cout << "\nStopping cache service...\n";
    manager.stop_service("cache");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
    
    // Restart service
    std::cout << "\nRestarting cache service...\n";
    manager.restart_service("cache");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
    
    // Stop all
    std::cout << "\nStopping all services...\n";
    manager.stop_all();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
}

void demo_startup_order() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Startup Order Demo                  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ServiceManager manager;
    
    manager.register_service("proxy", std::make_unique<ProxyServiceAdapter>(8080));
    manager.register_service("cache", std::make_unique<CacheServiceAdapter>(6379));
    manager.register_service("dns", std::make_unique<DNSServiceAdapter>(53));
    
    // Set startup order: dns -> cache -> proxy
    std::cout << "Setting startup order: dns → cache → proxy\n";
    manager.set_startup_order({"dns", "cache", "proxy"});
    
    // Set shutdown order: reverse
    std::cout << "Setting shutdown order: proxy → cache → dns\n";
    manager.set_shutdown_order({"proxy", "cache", "dns"});
    
    std::cout << "\nStarting all services in order...\n";
    manager.start_all();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_service_status(manager);
    
    std::cout << "\nStopping all services in order...\n";
    manager.stop_all();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_service_status(manager);
}

void demo_statistics() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Service Statistics Demo             ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ServiceManager manager;
    
    manager.register_service("cache", std::make_unique<CacheServiceAdapter>(6379));
    manager.register_service("dns", std::make_unique<DNSServiceAdapter>(53));
    manager.register_service("proxy", std::make_unique<ProxyServiceAdapter>(8080));
    
    std::cout << "Total services: " << manager.get_total_count() << "\n";
    std::cout << "Running services: " << manager.get_running_count() << "\n";
    
    manager.start_service("cache");
    manager.start_service("dns");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nAfter starting cache and dns:\n";
    std::cout << "Total services: " << manager.get_total_count() << "\n";
    std::cout << "Running services: " << manager.get_running_count() << "\n";
    
    manager.stop_all();
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       Server Launch & Management Demo             ║\n";
    std::cout << "║       Service Orchestration                       ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_service_registration();
    demo_service_lifecycle();
    demo_startup_order();
    demo_statistics();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    return 0;
}
