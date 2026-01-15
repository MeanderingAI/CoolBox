#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include "IO/data_structures/concurrent_hash_map.h"

namespace services {
namespace proxy {

// Forward proxy configuration
struct ProxyConfig {
    int port;
    bool enable_caching;
    bool enable_logging;
    size_t max_cache_size_mb;
    std::vector<std::string> blocked_domains;
    
    ProxyConfig() 
        : port(8080)
        , enable_caching(true)
        , enable_logging(true)
        , max_cache_size_mb(100)
        , blocked_domains() {}
};

// Cached response
struct CachedResponse {
    std::string headers;
    std::string body;
    std::chrono::steady_clock::time_point cached_at;
    size_t size_bytes;
    
    bool is_expired(int max_age_seconds) const {
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - cached_at).count();
        return age > max_age_seconds;
    }
};

// HTTP Proxy Server
class ProxyServer {
public:
    ProxyServer(const ProxyConfig& config = ProxyConfig());
    ~ProxyServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Configuration
    void set_config(const ProxyConfig& config);
    ProxyConfig get_config() const;
    
    // Access control
    void block_domain(const std::string& domain);
    void unblock_domain(const std::string& domain);
    bool is_blocked(const std::string& domain) const;
    
    // Cache management
    void clear_cache();
    size_t get_cache_size() const;
    void enable_caching(bool enable);
    
    // Statistics
    uint64_t get_requests_count() const;
    uint64_t get_cache_hits() const;
    uint64_t get_blocked_requests() const;
    
private:
    ProxyConfig config_;
    std::atomic<bool> running_;
    int server_socket_;
    std::thread server_thread_;
    
    // Cache storage
    std::unique_ptr<data_structures::ConcurrentHashMap<std::string, std::shared_ptr<CachedResponse>>> cache_;
    
    // Statistics
    std::atomic<uint64_t> requests_count_;
    std::atomic<uint64_t> cache_hits_;
    std::atomic<uint64_t> blocked_requests_;
    
    // Server operations
    void run_server();
    void handle_client(int client_socket);
    
    // Request handling
    std::string parse_request(const std::string& request, std::string& method, std::string& host, std::string& path);
    bool forward_request(const std::string& host, int port, const std::string& request, std::string& response);
    
    // Cache operations
    std::optional<CachedResponse> get_cached_response(const std::string& url);
    void cache_response(const std::string& url, const std::string& headers, const std::string& body);
};

// Reverse Proxy (Load Balancer)
class ReverseProxy {
public:
    ReverseProxy(int port = 80);
    ~ReverseProxy();
    
    // Backend management
    void add_backend(const std::string& host, int port);
    void remove_backend(const std::string& host, int port);
    std::vector<std::pair<std::string, int>> get_backends() const;
    
    // Load balancing strategies
    enum class Strategy {
        ROUND_ROBIN,
        LEAST_CONNECTIONS,
        IP_HASH,
        RANDOM
    };
    
    void set_strategy(Strategy strategy);
    Strategy get_strategy() const;
    
    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Health checks
    void enable_health_checks(bool enable, int interval_seconds = 30);
    bool is_backend_healthy(const std::string& host, int port);
    
    // Statistics
    uint64_t get_total_requests() const;
    uint64_t get_backend_requests(const std::string& host, int port) const;
    
private:
    int port_;
    std::atomic<bool> running_;
    Strategy strategy_;
    std::atomic<size_t> current_backend_index_;
    
    struct Backend {
        std::string host;
        int port;
        std::atomic<uint64_t> request_count;
        std::atomic<uint64_t> active_connections;
        std::atomic<bool> healthy;
        
        Backend(const std::string& h, int p) 
            : host(h), port(p), request_count(0), active_connections(0), healthy(true) {}
    };
    
    std::vector<std::shared_ptr<Backend>> backends_;
    std::mutex backends_mutex_;
    
    std::atomic<uint64_t> total_requests_;
    
    // Server operations
    void run_server();
    void handle_client(int client_socket);
    
    // Load balancing
    std::shared_ptr<Backend> select_backend(const std::string& client_ip);
    
    // Health checking
    void run_health_checks();
    bool check_backend_health(std::shared_ptr<Backend> backend);
};

} // namespace proxy
} // namespace services
