#pragma once

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <thread>
#include <atomic>

namespace services {
namespace server_launch {

// Service status
enum class ServiceStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    FAILED
};

// Base service interface
class IService {
public:
    virtual ~IService() = default;
    
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    virtual std::string get_name() const = 0;
    virtual std::string get_status_string() const = 0;
};

// Service wrapper for managing individual services
class ServiceWrapper {
public:
    ServiceWrapper(const std::string& name, std::unique_ptr<IService> service);
    ~ServiceWrapper();
    
    bool start();
    void stop();
    bool restart();
    
    ServiceStatus get_status() const;
    std::string get_name() const;
    std::string get_error_message() const;
    
    // Health check
    bool is_healthy() const;
    void set_health_check(std::function<bool()> check);
    
private:
    std::string name_;
    std::unique_ptr<IService> service_;
    std::atomic<ServiceStatus> status_;
    std::string error_message_;
    std::function<bool()> health_check_;
    std::thread health_check_thread_;
    std::atomic<bool> monitoring_;
    
    void run_health_checks();
};

// Service Manager - orchestrates multiple services
class ServiceManager {
public:
    ServiceManager();
    ~ServiceManager();
    
    // Service registration
    void register_service(const std::string& name, std::unique_ptr<IService> service);
    void unregister_service(const std::string& name);
    
    // Service control
    bool start_service(const std::string& name);
    void stop_service(const std::string& name);
    bool restart_service(const std::string& name);
    
    bool start_all();
    void stop_all();
    void restart_all();
    
    // Service queries
    ServiceStatus get_service_status(const std::string& name) const;
    std::vector<std::string> get_service_names() const;
    std::map<std::string, ServiceStatus> get_all_statuses() const;
    
    // Statistics
    size_t get_running_count() const;
    size_t get_total_count() const;
    
    // Configuration
    void set_startup_order(const std::vector<std::string>& order);
    void set_shutdown_order(const std::vector<std::string>& order);
    
    // Monitoring
    void enable_auto_restart(bool enable);
    bool is_auto_restart_enabled() const;
    
private:
    std::map<std::string, std::unique_ptr<ServiceWrapper>> services_;
    std::vector<std::string> startup_order_;
    std::vector<std::string> shutdown_order_;
    std::atomic<bool> auto_restart_;
    mutable std::mutex services_mutex_;
};

// Service configuration
struct ServiceConfig {
    std::string name;
    std::string type;  // "cache", "dns", "proxy", etc.
    int port;
    bool auto_start;
    std::map<std::string, std::string> parameters;
    
    ServiceConfig() : port(0), auto_start(true) {}
};

// Configuration-based service launcher
class ServiceLauncher {
public:
    ServiceLauncher();
    ~ServiceLauncher();
    
    // Load configuration
    bool load_config(const std::string& config_file);
    bool load_config_string(const std::string& config_json);
    
    // Service creation
    bool create_services_from_config();
    
    // Get the service manager
    ServiceManager& get_manager();
    
private:
    ServiceManager manager_;
    std::vector<ServiceConfig> configs_;
    
    std::unique_ptr<IService> create_service(const ServiceConfig& config);
};

// Utility functions
std::string status_to_string(ServiceStatus status);
ServiceStatus string_to_status(const std::string& status);

} // namespace server_launch
} // namespace services
