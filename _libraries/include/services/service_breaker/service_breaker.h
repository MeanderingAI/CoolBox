#ifndef SERVICE_BREAKER_H
#define SERVICE_BREAKER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>

namespace services {

struct ServiceConfig {
    std::string name;
    std::string description;
    int port;
    bool is_running;
    std::string status_message;
    time_t last_started;
    time_t last_stopped;
};

class ServiceBreaker {
public:
    ServiceBreaker();
    ~ServiceBreaker() = default;

    // Service control
    bool start_service(const std::string& service_name);
    bool stop_service(const std::string& service_name);
    bool set_port(const std::string& service_name, int port);
    bool is_running(const std::string& service_name) const;
    
    // Service information
    ServiceConfig get_service_config(const std::string& service_name) const;
    std::unordered_map<std::string, ServiceConfig> get_all_services() const;
    
    // Service registration
    void register_service(const std::string& name, const std::string& description, int default_port);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ServiceConfig> services_;
    
    void init_default_services();
};

} // namespace services

#endif // SERVICE_BREAKER_H
