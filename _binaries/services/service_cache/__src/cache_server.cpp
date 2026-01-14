#include "cache_server.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// Library metadata
extern "C" {
    __attribute__((visibility("default"), used))
    const char* get_library_name() { return "cache_server"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_version() { return "2.1.0"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_description() { return "High-performance Redis-compatible caching system with LRU eviction, TTL support, and pub/sub messaging"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_author() { return "ToolBox Team"; }
}

namespace services {

// Command parsing
Command Command::parse(const std::string& input) {
    Command cmd;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        cmd.args.push_back(token);
    }
    
    if (cmd.args.empty()) {
        cmd.type = CommandType::UNKNOWN;
        return cmd;
    }
    
    std::string command = cmd.args[0];
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    
    if (command == "SET") cmd.type = CommandType::SET;
    else if (command == "GET") cmd.type = CommandType::GET;
    else if (command == "DEL") cmd.type = CommandType::DEL;
    else if (command == "EXISTS") cmd.type = CommandType::EXISTS;
    else if (command == "LPUSH") cmd.type = CommandType::LPUSH;
    else if (command == "RPUSH") cmd.type = CommandType::RPUSH;
    else if (command == "LPOP") cmd.type = CommandType::LPOP;
    else if (command == "RPOP") cmd.type = CommandType::RPOP;
    else if (command == "LLEN") cmd.type = CommandType::LLEN;
    else if (command == "LRANGE") cmd.type = CommandType::LRANGE;
    else if (command == "SADD") cmd.type = CommandType::SADD;
    else if (command == "SREM") cmd.type = CommandType::SREM;
    else if (command == "SISMEMBER") cmd.type = CommandType::SISMEMBER;
    else if (command == "SCARD") cmd.type = CommandType::SCARD;
    else if (command == "SMEMBERS") cmd.type = CommandType::SMEMBERS;
    else if (command == "KEYS") cmd.type = CommandType::KEYS;
    else if (command == "FLUSH" || command == "FLUSHDB") cmd.type = CommandType::FLUSH;
    else if (command == "DBSIZE") cmd.type = CommandType::DBSIZE;
    else if (command == "INCR") cmd.type = CommandType::INCR;
    else if (command == "DECR") cmd.type = CommandType::DECR;
    else if (command == "INCRBY") cmd.type = CommandType::INCRBY;
    else if (command == "EXPIRE") cmd.type = CommandType::EXPIRE;
    else if (command == "TTL") cmd.type = CommandType::TTL;
    else if (command == "PERSIST") cmd.type = CommandType::PERSIST;
    else if (command == "PING") cmd.type = CommandType::PING;
    else if (command == "QUIT") cmd.type = CommandType::QUIT;
    else cmd.type = CommandType::UNKNOWN;
    
    return cmd;
}

// CacheServer implementation
CacheServer::CacheServer(int port) 
    : port_(port)
    , running_(false)
    , cache_(std::make_unique<DistributedCache>())
    , server_socket_(-1) {
}

CacheServer::~CacheServer() {
    stop();
}

bool CacheServer::start() {
    if (running_) {
        return false;
    }
    
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Listen
    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&CacheServer::run_server, this);
    
    return true;
}

void CacheServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

bool CacheServer::is_running() const {
    return running_;
}

DistributedCache& CacheServer::get_cache() {
    return *cache_;
}

std::string CacheServer::handle_command(const std::string& command_line) {
    Command cmd = Command::parse(command_line);
    return execute_command(cmd);
}

void CacheServer::run_server() {
    while (running_) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_socket = accept(server_socket_, 
                                   (struct sockaddr*)&client_address, 
                                   &client_len);
        
        if (client_socket < 0) {
            if (running_) {
                continue;
            }
            break;
        }
        
        // Handle client in separate thread
        std::thread(&CacheServer::handle_client, this, client_socket).detach();
    }
}

void CacheServer::handle_client(int client_socket) {
    char buffer[4096];
    
    while (running_) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            break;
        }
        
        std::string command_line(buffer, bytes_read);
        std::string response = handle_command(command_line);
        
        send(client_socket, response.c_str(), response.length(), 0);
        
        Command cmd = Command::parse(command_line);
        if (cmd.type == CommandType::QUIT) {
            break;
        }
    }
    
    close(client_socket);
}

std::string CacheServer::execute_command(const Command& cmd) {
    try {
        switch (cmd.type) {
            case CommandType::SET: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for SET");
                }
                if (cmd.args.size() >= 4) {
                    int ttl = std::stoi(cmd.args[3]);
                    cache_->set(cmd.args[1], cmd.args[2], ttl);
                } else {
                    cache_->set(cmd.args[1], cmd.args[2]);
                }
                return format_ok();
            }
            
            case CommandType::GET: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for GET");
                }
                auto value = cache_->get(cmd.args[1]);
                if (value) {
                    return format_string(*value);
                }
                return format_null();
            }
            
            case CommandType::DEL: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for DEL");
                }
                bool deleted = cache_->del(cmd.args[1]);
                return format_integer(deleted ? 1 : 0);
            }
            
            case CommandType::EXISTS: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for EXISTS");
                }
                bool exists = cache_->exists(cmd.args[1]);
                return format_integer(exists ? 1 : 0);
            }
            
            case CommandType::LPUSH: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for LPUSH");
                }
                cache_->lpush(cmd.args[1], cmd.args[2]);
                return format_integer(cache_->llen(cmd.args[1]));
            }
            
            case CommandType::RPUSH: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for RPUSH");
                }
                cache_->rpush(cmd.args[1], cmd.args[2]);
                return format_integer(cache_->llen(cmd.args[1]));
            }
            
            case CommandType::LPOP: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for LPOP");
                }
                auto value = cache_->lpop(cmd.args[1]);
                if (value) {
                    return format_string(*value);
                }
                return format_null();
            }
            
            case CommandType::RPOP: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for RPOP");
                }
                auto value = cache_->rpop(cmd.args[1]);
                if (value) {
                    return format_string(*value);
                }
                return format_null();
            }
            
            case CommandType::LLEN: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for LLEN");
                }
                size_t len = cache_->llen(cmd.args[1]);
                return format_integer(len);
            }
            
            case CommandType::SADD: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for SADD");
                }
                cache_->sadd(cmd.args[1], cmd.args[2]);
                return format_integer(1);
            }
            
            case CommandType::SREM: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for SREM");
                }
                bool removed = cache_->srem(cmd.args[1], cmd.args[2]);
                return format_integer(removed ? 1 : 0);
            }
            
            case CommandType::SISMEMBER: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for SISMEMBER");
                }
                bool is_member = cache_->sismember(cmd.args[1], cmd.args[2]);
                return format_integer(is_member ? 1 : 0);
            }
            
            case CommandType::SCARD: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for SCARD");
                }
                size_t card = cache_->scard(cmd.args[1]);
                return format_integer(card);
            }
            
            case CommandType::SMEMBERS: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for SMEMBERS");
                }
                auto members = cache_->smembers(cmd.args[1]);
                return format_array(members);
            }
            
            case CommandType::KEYS: {
                auto keys = cache_->keys();
                return format_array(keys);
            }
            
            case CommandType::FLUSH: {
                cache_->flush();
                return format_ok();
            }
            
            case CommandType::DBSIZE: {
                size_t size = cache_->dbsize();
                return format_integer(size);
            }
            
            case CommandType::INCR: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for INCR");
                }
                auto value = cache_->incr(cmd.args[1]);
                if (value) {
                    return format_integer(*value);
                }
                return format_error("value is not an integer");
            }
            
            case CommandType::DECR: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for DECR");
                }
                auto value = cache_->decr(cmd.args[1]);
                if (value) {
                    return format_integer(*value);
                }
                return format_error("value is not an integer");
            }
            
            case CommandType::INCRBY: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for INCRBY");
                }
                int64_t increment = std::stoll(cmd.args[2]);
                auto value = cache_->incrby(cmd.args[1], increment);
                if (value) {
                    return format_integer(*value);
                }
                return format_error("value is not an integer");
            }
            
            case CommandType::EXPIRE: {
                if (cmd.args.size() < 3) {
                    return format_error("wrong number of arguments for EXPIRE");
                }
                int seconds = std::stoi(cmd.args[2]);
                bool result = cache_->expire(cmd.args[1], seconds);
                return format_integer(result ? 1 : 0);
            }
            
            case CommandType::TTL: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for TTL");
                }
                auto ttl = cache_->ttl(cmd.args[1]);
                if (ttl) {
                    return format_integer(*ttl);
                }
                return format_integer(-2);
            }
            
            case CommandType::PERSIST: {
                if (cmd.args.size() < 2) {
                    return format_error("wrong number of arguments for PERSIST");
                }
                bool result = cache_->persist(cmd.args[1]);
                return format_integer(result ? 1 : 0);
            }
            
            case CommandType::PING: {
                return "+PONG\r\n";
            }
            
            case CommandType::QUIT: {
                return format_ok();
            }
            
            default:
                return format_error("unknown command");
        }
    } catch (const std::exception& e) {
        return format_error(e.what());
    }
}

std::string CacheServer::format_ok() {
    return "+OK\r\n";
}

std::string CacheServer::format_error(const std::string& message) {
    return "-ERR " + message + "\r\n";
}

std::string CacheServer::format_string(const std::string& value) {
    return "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
}

std::string CacheServer::format_integer(int64_t value) {
    return ":" + std::to_string(value) + "\r\n";
}

std::string CacheServer::format_array(const std::vector<std::string>& values) {
    std::string result = "*" + std::to_string(values.size()) + "\r\n";
    for (const auto& value : values) {
        result += format_string(value);
    }
    return result;
}

std::string CacheServer::format_null() {
    return "$-1\r\n";
}

// CacheClient implementation
CacheClient::CacheClient(const std::string& host, int port)
    : host_(host)
    , port_(port)
    , socket_(-1)
    , connected_(false) {
}

CacheClient::~CacheClient() {
    disconnect();
}

bool CacheClient::connect() {
    if (connected_) {
        return true;
    }
    
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        return false;
    }
    
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host_.c_str(), &server_address.sin_addr) <= 0) {
        close(socket_);
        return false;
    }
    
    if (::connect(socket_, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        close(socket_);
        return false;
    }
    
    connected_ = true;
    return true;
}

void CacheClient::disconnect() {
    if (connected_) {
        close(socket_);
        connected_ = false;
    }
}

bool CacheClient::is_connected() const {
    return connected_;
}

std::string CacheClient::send_command(const std::string& command) {
    if (!connected_) {
        return "";
    }
    
    send(socket_, command.c_str(), command.length(), 0);
    
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return "";
    }
    
    return std::string(buffer, bytes_read);
}

bool CacheClient::set(const std::string& key, const std::string& value) {
    std::string cmd = "SET " + key + " " + value + "\r\n";
    std::string response = send_command(cmd);
    return response.find("+OK") == 0;
}

std::optional<std::string> CacheClient::get(const std::string& key) {
    std::string cmd = "GET " + key + "\r\n";
    std::string response = send_command(cmd);
    
    if (response.find("$-1") == 0) {
        return std::nullopt;
    }
    
    // Parse response (simplified)
    size_t start = response.find("\r\n");
    if (start == std::string::npos) {
        return std::nullopt;
    }
    
    start += 2;
    size_t end = response.find("\r\n", start);
    if (end == std::string::npos) {
        return std::nullopt;
    }
    
    return response.substr(start, end - start);
}

bool CacheClient::del(const std::string& key) {
    std::string cmd = "DEL " + key + "\r\n";
    std::string response = send_command(cmd);
    return response.find(":1") == 0;
}

} // namespace services
