#include "../headers/service_registry.h"

#include <algorithm>

namespace distributed {

bool ServiceRegistry::register_service(const std::string&     service_name,
                                        const std::string&     instance_id,
                                        const ServiceEndpoint& endpoint) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& instances = services_[service_name];
    if (instances.count(instance_id)) return false;
    const auto now = Clock::now();
    instances[instance_id] = {service_name, instance_id, endpoint, now, now, true};
    return true;
}

bool ServiceRegistry::deregister(const std::string& service_name,
                                  const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto svc_it = services_.find(service_name);
    if (svc_it == services_.end()) return false;
    return svc_it->second.erase(instance_id) > 0;
}

bool ServiceRegistry::heartbeat(const std::string& service_name,
                                 const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto svc_it = services_.find(service_name);
    if (svc_it == services_.end()) return false;
    auto inst_it = svc_it->second.find(instance_id);
    if (inst_it == svc_it->second.end()) return false;
    inst_it->second.last_heartbeat = Clock::now();
    inst_it->second.healthy        = true;
    return true;
}

std::size_t ServiceRegistry::expire_stale(std::chrono::milliseconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto cutoff = Clock::now() - ttl;
    std::size_t expired = 0;
    for (auto& [svc_name, instances] : services_) {
        for (auto& [id, rec] : instances) {
            if (rec.healthy && rec.last_heartbeat < cutoff) {
                rec.healthy = false;
                ++expired;
            }
        }
    }
    return expired;
}

std::vector<ServiceRecord>
ServiceRegistry::lookup(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto svc_it = services_.find(service_name);
    if (svc_it == services_.end()) return {};

    std::vector<ServiceRecord> results;
    for (const auto& [id, rec] : svc_it->second) {
        if (rec.healthy) results.push_back(rec);
    }
    return results;
}

std::optional<ServiceRecord>
ServiceRegistry::get_instance(const std::string& service_name,
                               const std::string& instance_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto svc_it = services_.find(service_name);
    if (svc_it == services_.end()) return std::nullopt;
    auto inst_it = svc_it->second.find(instance_id);
    if (inst_it == svc_it->second.end()) return std::nullopt;
    return inst_it->second;
}

std::size_t ServiceRegistry::instance_count(const std::string& service_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto svc_it = services_.find(service_name);
    return (svc_it == services_.end()) ? 0 : svc_it->second.size();
}

void ServiceRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
}

} // namespace distributed
