#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include "concurrent_hash_map.h"

namespace services {
namespace dns {

// DNS record types
enum class RecordType {
    A,      // IPv4 address
    AAAA,   // IPv6 address
    CNAME,  // Canonical name (alias)
    MX,     // Mail exchange
    TXT,    // Text record
    NS,     // Name server
    PTR     // Pointer (reverse DNS)
};

// DNS record entry
struct DNSRecord {
    std::string name;
    RecordType type;
    std::string value;
    uint32_t ttl;  // Time to live in seconds
    std::chrono::steady_clock::time_point created;
    
    DNSRecord(const std::string& n, RecordType t, const std::string& v, uint32_t ttl_val)
        : name(n), type(t), value(v), ttl(ttl_val), created(std::chrono::steady_clock::now()) {}
    
    bool is_expired() const {
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - created).count();
        return age > ttl;
    }
};

// DNS Server
class DNSServer {
public:
    DNSServer(int port = 53);
    ~DNSServer();
    
    // Record management
    bool add_record(const std::string& name, RecordType type, const std::string& value, uint32_t ttl = 3600);
    bool remove_record(const std::string& name, RecordType type);
    std::optional<std::string> resolve(const std::string& name, RecordType type = RecordType::A);
    std::vector<DNSRecord> get_all_records(const std::string& name);
    
    // Server operations
    bool start();
    void stop();
    bool is_running() const;
    
    // Cache management
    void clear_cache();
    void cleanup_expired();
    size_t cache_size() const;
    
    // Statistics
    uint64_t get_queries_count() const;
    uint64_t get_cache_hits() const;
    uint64_t get_cache_misses() const;
    
private:
    int port_;
    bool running_;
    
    // Storage for DNS records
    std::unique_ptr<data_structures::ConcurrentHashMap<std::string, std::vector<DNSRecord>>> records_;
    
    // Statistics
    std::atomic<uint64_t> queries_count_;
    std::atomic<uint64_t> cache_hits_;
    std::atomic<uint64_t> cache_misses_;
    
    // Helper methods
    std::string record_key(const std::string& name, RecordType type) const;
    std::string record_type_to_string(RecordType type) const;
};

// DNS Client
class DNSClient {
public:
    DNSClient(const std::string& server = "8.8.8.8", int port = 53);
    ~DNSClient();
    
    // Query operations
    std::optional<std::string> resolve(const std::string& hostname);
    std::optional<std::string> resolve_ipv6(const std::string& hostname);
    std::vector<std::string> resolve_all(const std::string& hostname);
    
    // Reverse DNS lookup
    std::optional<std::string> reverse_lookup(const std::string& ip);
    
    // Configuration
    void set_server(const std::string& server, int port = 53);
    void set_timeout(int seconds);
    
private:
    std::string server_;
    int port_;
    int timeout_;
};

} // namespace dns
} // namespace services
