#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include "distributed_cache.h"

namespace services {

// Simple cache server protocol
enum class CommandType {
    SET,
    GET,
    DEL,
    EXISTS,
    LPUSH,
    RPUSH,
    LPOP,
    RPOP,
    LLEN,
    LRANGE,
    SADD,
    SREM,
    SISMEMBER,
    SCARD,
    SMEMBERS,
    KEYS,
    FLUSH,
    DBSIZE,
    INCR,
    DECR,
    INCRBY,
    EXPIRE,
    TTL,
    PERSIST,
    PING,
    QUIT,
    UNKNOWN
};

// Command parser
struct Command {
    CommandType type;
    std::vector<std::string> args;
    
    static Command parse(const std::string& input);
};

// Cache server (TCP-based)
class CacheServer {
public:
    CacheServer(int port = 6379);
    ~CacheServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Get underlying cache
    DistributedCache& get_cache();
    
    // Handle a single command (for testing or direct use)
    std::string handle_command(const std::string& command_line);
    
private:
    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<DistributedCache> cache_;
    std::thread server_thread_;
    int server_socket_;
    
    // Server operations
    void run_server();
    void handle_client(int client_socket);
    std::string execute_command(const Command& cmd);
    
    // Response formatters
    std::string format_ok();
    std::string format_error(const std::string& message);
    std::string format_string(const std::string& value);
    std::string format_integer(int64_t value);
    std::string format_array(const std::vector<std::string>& values);
    std::string format_null();
};

// Simple cache client
class CacheClient {
public:
    CacheClient(const std::string& host = "localhost", int port = 6379);
    ~CacheClient();
    
    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const;
    
    // Send command and get response
    std::string send_command(const std::string& command);
    
    // Convenience methods
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    
private:
    std::string host_;
    int port_;
    int socket_;
    bool connected_;
};

} // namespace services
