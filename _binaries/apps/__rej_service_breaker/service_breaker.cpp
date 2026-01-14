#include "services/service_breaker/service_breaker.h"
#include <iostream>

// Library metadata
extern "C" {
    __attribute__((weak, used))
    const char* get_library_name_service_breaker() { return "service_breaker"; }
    
    __attribute__((weak, used))
    const char* get_library_version_service_breaker() { return "1.0.0"; }
    
    __attribute__((weak, used))
    const char* get_library_description_service_breaker() { return "Service registry and circuit breaker for managing microservices with health checks and failover"; }
    
    __attribute__((weak, used))
    const char* get_library_author_service_breaker() { return "ToolBox Team"; }
}

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
        return false;
    }
    
    it->second.is_running = true;
    it->second.status_message = "Running";
    it->second.last_started = std::time(nullptr);
    return true;
}

bool ServiceBreaker::stop_service(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(service_name);
    if (it == services_.end() || !it->second.is_running) {
        return false;
    }
    it->second.is_running = false;
    it->second.status_message = "Stopped";
    it->second.last_stopped = std::time(nullptr);
    return true;
}

ServiceConfig ServiceBreaker::get_service_config(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(service_name);
    if (it != services_.end()) {
        return it->second;
    }
    return ServiceConfig{};
}



bool ServiceBreaker::set_port(const std::string& service_name, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(service_name);
    if (it != services_.end()) {
        it->second.port = port;
        return true;
    }
    return false;
}

bool ServiceBreaker::is_running(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(service_name);
    return it != services_.end() && it->second.is_running;
}

std::unordered_map<std::string, ServiceConfig> ServiceBreaker::get_all_services() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_;
}

} // namespace services
