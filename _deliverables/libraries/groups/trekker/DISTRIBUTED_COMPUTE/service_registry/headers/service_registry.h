#ifndef DISTRIBUTED_SERVICE_REGISTRY_H
#define DISTRIBUTED_SERVICE_REGISTRY_H

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace distributed {

struct ServiceEndpoint {
    std::string   host;
    std::uint16_t port;
    std::string   metadata; // optional key=value or JSON blob
};

struct ServiceRecord {
    std::string                           service_name;
    std::string                           instance_id;
    ServiceEndpoint                       endpoint;
    std::chrono::steady_clock::time_point registered_at;
    std::chrono::steady_clock::time_point last_heartbeat;
    bool                                  healthy;
};

// In-process service registry providing registration, lookup,
// heartbeat tracking, and TTL-based health expiry.
class ServiceRegistry {
public:
    using Clock = std::chrono::steady_clock;

    // Register an instance. Returns false if instance_id already exists.
    bool register_service(const std::string&     service_name,
                          const std::string&     instance_id,
                          const ServiceEndpoint& endpoint);

    // Remove an instance. Returns false if not found.
    bool deregister(const std::string& service_name,
                    const std::string& instance_id);

    // Record a heartbeat; marks instance healthy. Returns false if not found.
    bool heartbeat(const std::string& service_name,
                   const std::string& instance_id);

    // Mark instances whose last heartbeat is older than `ttl` as unhealthy.
    // Returns the count of newly expired instances.
    std::size_t expire_stale(std::chrono::milliseconds ttl);

    // Return all healthy instances for a service.
    std::vector<ServiceRecord> lookup(const std::string& service_name) const;

    // Return a specific instance by ID (healthy or not).
    std::optional<ServiceRecord> get_instance(const std::string& service_name,
                                              const std::string& instance_id) const;

    // Total registered instances (healthy + unhealthy).
    std::size_t instance_count(const std::string& service_name) const;

    void clear();

private:
    using InstanceMap = std::unordered_map<std::string, ServiceRecord>;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, InstanceMap> services_;
};

} // namespace distributed

#endif // DISTRIBUTED_SERVICE_REGISTRY_H
