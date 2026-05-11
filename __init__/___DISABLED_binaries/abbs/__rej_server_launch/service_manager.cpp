#include "services/server_launch/service_manager.h"
#include <algorithm>
#include <chrono>

namespace services {
namespace server_launch {

// ServiceWrapper implementation
ServiceWrapper::ServiceWrapper(const std::string& name, std::unique_ptr<IService> service)
    : name_(name)
    , service_(std::move(service))
    , status_(ServiceStatus::STOPPED)
    , monitoring_(false) {
}

ServiceWrapper::~ServiceWrapper() {
    if (monitoring_) {
        monitoring_ = false;
        if (health_check_thread_.joinable()) {
            health_check_thread_.join();
        }
    }
    stop();
}

bool ServiceWrapper::start() {
    if (status_ == ServiceStatus::RUNNING) {
        return true;
    }
    
    status_ = ServiceStatus::STARTING;
    
    try {
        if (service_->start()) {
            status_ = ServiceStatus::RUNNING;
            error_message_.clear();
            
            // Start health monitoring if check is configured
            if (health_check_ && !monitoring_) {
                monitoring_ = true;
                health_check_thread_ = std::thread(&ServiceWrapper::run_health_checks, this);
            }
            
            return true;
        } else {
            status_ = ServiceStatus::FAILED;
            error_message_ = "Service failed to start";
            return false;
        }
    } catch (const std::exception& e) {
        status_ = ServiceStatus::FAILED;
        error_message_ = std::string("Exception during start: ") + e.what();
        return false;
    }
}

void ServiceWrapper::stop() {
    if (status_ == ServiceStatus::STOPPED) {
        return;
    }
    
    status_ = ServiceStatus::STOPPING;
    
    try {
        service_->stop();
        status_ = ServiceStatus::STOPPED;
    } catch (const std::exception& e) {
        error_message_ = std::string("Exception during stop: ") + e.what();
        status_ = ServiceStatus::STOPPED;
    }
}

bool ServiceWrapper::restart() {
    stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return start();
}

ServiceStatus ServiceWrapper::get_status() const {
    return status_.load();
}

std::string ServiceWrapper::get_name() const {
    return name_;
}

std::string ServiceWrapper::get_error_message() const {
    return error_message_;
}

bool ServiceWrapper::is_healthy() const {
    if (!health_check_) {
        return status_ == ServiceStatus::RUNNING;
    }
    
    try {
        return health_check_();
    } catch (...) {
        return false;
    }
}

void ServiceWrapper::set_health_check(std::function<bool()> check) {
    health_check_ = check;
}

void ServiceWrapper::run_health_checks() {
    while (monitoring_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (status_ == ServiceStatus::RUNNING) {
            if (!is_healthy()) {
                status_ = ServiceStatus::FAILED;
                error_message_ = "Health check failed";
            }
        }
    }
}

// ServiceManager implementation
ServiceManager::ServiceManager()
    : auto_restart_(false) {
}

ServiceManager::~ServiceManager() {
    stop_all();
}

void ServiceManager::register_service(const std::string& name, std::unique_ptr<IService> service) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    services_[name] = std::make_unique<ServiceWrapper>(name, std::move(service));
}

void ServiceManager::unregister_service(const std::string& name) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        it->second->stop();
        services_.erase(it);
    }
}

bool ServiceManager::start_service(const std::string& name) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second->start();
    }
    return false;
}

void ServiceManager::stop_service(const std::string& name) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        it->second->stop();
    }
}

bool ServiceManager::restart_service(const std::string& name) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second->restart();
    }
    return false;
}

bool ServiceManager::start_all() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    bool all_started = true;
    
    if (!startup_order_.empty()) {
        // Start in specified order
        for (const auto& name : startup_order_) {
            auto it = services_.find(name);
            if (it != services_.end()) {
                if (!it->second->start()) {
                    all_started = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } else {
        // Start all services
        for (auto& pair : services_) {
            if (!pair.second->start()) {
                all_started = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    return all_started;
}

void ServiceManager::stop_all() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    if (!shutdown_order_.empty()) {
        // Stop in specified order
        for (const auto& name : shutdown_order_) {
            auto it = services_.find(name);
            if (it != services_.end()) {
                it->second->stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } else {
        // Stop all services in reverse registration order
        for (auto it = services_.rbegin(); it != services_.rend(); ++it) {
            it->second->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void ServiceManager::restart_all() {
    stop_all();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    start_all();
}

ServiceStatus ServiceManager::get_service_status(const std::string& name) const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second->get_status();
    }
    return ServiceStatus::STOPPED;
}

std::vector<std::string> ServiceManager::get_service_names() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    std::vector<std::string> names;
    for (const auto& pair : services_) {
        names.push_back(pair.first);
    }
    return names;
}

std::map<std::string, ServiceStatus> ServiceManager::get_all_statuses() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    std::map<std::string, ServiceStatus> statuses;
    for (const auto& pair : services_) {
        statuses[pair.first] = pair.second->get_status();
    }
    return statuses;
}

size_t ServiceManager::get_running_count() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    size_t count = 0;
    for (const auto& pair : services_) {
        if (pair.second->get_status() == ServiceStatus::RUNNING) {
            count++;
        }
    }
    return count;
}

size_t ServiceManager::get_total_count() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    return services_.size();
}

void ServiceManager::set_startup_order(const std::vector<std::string>& order) {
    startup_order_ = order;
}

void ServiceManager::set_shutdown_order(const std::vector<std::string>& order) {
    shutdown_order_ = order;
}

void ServiceManager::enable_auto_restart(bool enable) {
    auto_restart_ = enable;
}

bool ServiceManager::is_auto_restart_enabled() const {
    return auto_restart_.load();
}

// ServiceLauncher implementation
ServiceLauncher::ServiceLauncher() {
}

ServiceLauncher::~ServiceLauncher() {
}

bool ServiceLauncher::load_config(const std::string& config_file) {
    // Would implement JSON/YAML config file parsing
    return false;
}

bool ServiceLauncher::load_config_string(const std::string& config_json) {
    // Would implement JSON string parsing
    return false;
}

bool ServiceLauncher::create_services_from_config() {
    // Would create services based on loaded configs
    return false;
}

ServiceManager& ServiceLauncher::get_manager() {
    return manager_;
}

std::unique_ptr<IService> ServiceLauncher::create_service(const ServiceConfig& config) {
    // Would create specific service types based on config
    return nullptr;
}

// Utility functions
std::string status_to_string(ServiceStatus status) {
    switch (status) {
        case ServiceStatus::STOPPED: return "STOPPED";
        case ServiceStatus::STARTING: return "STARTING";
        case ServiceStatus::RUNNING: return "RUNNING";
        case ServiceStatus::STOPPING: return "STOPPING";
        case ServiceStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

ServiceStatus string_to_status(const std::string& status) {
    if (status == "STOPPED") return ServiceStatus::STOPPED;
    if (status == "STARTING") return ServiceStatus::STARTING;
    if (status == "RUNNING") return ServiceStatus::RUNNING;
    if (status == "STOPPING") return ServiceStatus::STOPPING;
    if (status == "FAILED") return ServiceStatus::FAILED;
    return ServiceStatus::STOPPED;
}

} // namespace server_launch
} // namespace services
