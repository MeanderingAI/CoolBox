#include "services/service_breaker/service_breaker.h"
#include <iostream>

namespace services {

ServiceBreaker::ServiceBreaker() {
    init_default_services();
}

void ServiceBreaker::init_default_services() {
    register_service("cache", "High-performance caching service", 6379);
    register_service("dfs", "Distributed file storage", 8080);
    register_service("mail", "SMTP/POP3 email server", 25);
    register_service("ml", "Machine learning server", 5000);
    register_service("security", "Malware detection scanner", 8888);
    register_service("dns", "Domain name resolution", 53);
    register_service("proxy", "HTTP/HTTPS proxy", 8080);
    register_service("urlshort", "URL shortening service", 9001);
    register_service("monitor", "System monitoring", 9002);
}

void ServiceBreaker::register_service(const std::string& name, const std::string& description, int default_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ServiceConfig config;
    config.name = name;
    config.description = description;
    config.port = default_port;
    config.is_running = false;
    config.status_message = "Stopped";
    config.last_started = 0;
    config.last_stopped = 0;
    
    services_[name] = config;
}

bool ServiceBreaker::start_service(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return false;
    }
    
    if (it->second.is_running) {
        it->second.status_message = "Already running";
        return true;
    }
    
    // Simulate service startup
    it->second.is_running = true;
    it->second.last_started = std::time(nullptr);
    it->second.status_message = "Running on port " + std::to_string(it->second.port);
    
    std::cout << "[ServiceBreaker] Started " << service_name << " on port " << it->second.port << std::endl;
    return true;
}

bool ServiceBreaker::stop_service(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return false;
    }
    
    if (!it->second.is_running) {
        it->second.status_message = "Already stopped";
        return true;
    }
    
    // Simulate service shutdown
    it->second.is_running = false;
    it->second.last_stopped = std::time(nullptr);
    it->second.status_message = "Stopped";
    
    std::cout << "[ServiceBreaker] Stopped " << service_name << std::endl;
    return true;
}

bool ServiceBreaker::set_port(const std::string& service_name, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return false;
    }
    
    if (port < 1 || port > 65535) {
        return false;
    }
    
    bool was_running = it->second.is_running;
    it->second.port = port;
    
    if (was_running) {
        it->second.status_message = "Running on port " + std::to_string(port);
    }
    
    std::cout << "[ServiceBreaker] Set " << service_name << " port to " << port << std::endl;
    return true;
}

bool ServiceBreaker::is_running(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return false;
    }
    
    return it->second.is_running;
}

ServiceConfig ServiceBreaker::get_service_config(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return ServiceConfig{};
    }
    
    return it->second;
}

std::unordered_map<std::string, ServiceConfig> ServiceBreaker::get_all_services() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_;
}

} // namespace services
