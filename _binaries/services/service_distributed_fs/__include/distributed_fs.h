#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <atomic>
#include <thread>

namespace services {
namespace distributed_fs {

// File chunk information
struct FileChunk {
    std::string chunk_id;
    size_t chunk_index;
    size_t size;
    std::string checksum;
    std::vector<std::string> replica_nodes;  // Node IDs where this chunk is stored
    
    FileChunk() : chunk_index(0), size(0) {}
};

// File metadata
struct FileMetadata {
    std::string file_id;
    std::string filename;
    std::string path;
    size_t total_size;
    size_t chunk_size;
    size_t num_chunks;
    int replication_factor;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    std::vector<FileChunk> chunks;
    std::map<std::string, std::string> attributes;
    
    FileMetadata() : total_size(0), chunk_size(0), num_chunks(0), replication_factor(3) {}
};

// Storage node information
struct StorageNodeInfo {
    std::string node_id;
    std::string address;
    int port;
    size_t capacity;
    size_t used_space;
    size_t available_space;
    bool is_alive;
    std::chrono::system_clock::time_point last_heartbeat;
    
    StorageNodeInfo() : port(0), capacity(0), used_space(0), available_space(0), is_alive(false) {}
    
    size_t get_available_space() const { return available_space; }
    double get_usage_ratio() const { 
        return capacity > 0 ? static_cast<double>(used_space) / capacity : 0.0; 
    }
};

// File operation result
struct FileOperationResult {
    bool success;
    std::string message;
    std::string file_id;
    size_t bytes_processed;
    
    FileOperationResult() : success(false), bytes_processed(0) {}
};

// Chunk data
struct ChunkData {
    std::string chunk_id;
    std::vector<char> data;
    std::string checksum;
    
    ChunkData() = default;
    ChunkData(const std::string& id, const std::vector<char>& d) 
        : chunk_id(id), data(d) {}
};

// Master node - manages metadata and coordinates operations
class MasterNode {
public:
    MasterNode(int port = 9000);
    ~MasterNode();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // File operations
    FileOperationResult create_file(const std::string& path, const std::vector<char>& data);
    FileOperationResult read_file(const std::string& path, std::vector<char>& data);
    FileOperationResult delete_file(const std::string& path);
    FileOperationResult update_file(const std::string& path, const std::vector<char>& data);
    
    // Directory operations
    std::vector<std::string> list_files(const std::string& directory = "/");
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path);
    
    // Metadata operations
    FileMetadata* get_file_metadata(const std::string& path);
    std::vector<FileMetadata> get_all_metadata();
    
    // Node management
    bool register_node(const StorageNodeInfo& node);
    bool unregister_node(const std::string& node_id);
    std::vector<StorageNodeInfo> get_active_nodes();
    void update_node_heartbeat(const std::string& node_id);
    
    // Configuration
    void set_chunk_size(size_t size) { chunk_size_ = size; }
    void set_replication_factor(int factor) { replication_factor_ = factor; }
    size_t get_chunk_size() const { return chunk_size_; }
    int get_replication_factor() const { return replication_factor_; }
    
    // Statistics
    size_t get_total_files() const;
    size_t get_total_size() const;
    size_t get_total_nodes() const;
    
private:
    int port_;
    std::atomic<bool> running_;
    size_t chunk_size_;
    int replication_factor_;
    
    // Metadata storage
    std::map<std::string, FileMetadata> file_metadata_;
    mutable std::mutex metadata_mutex_;
    
    // Node registry
    std::map<std::string, StorageNodeInfo> storage_nodes_;
    mutable std::mutex nodes_mutex_;
    
    // Chunk storage (in-memory for now, would be distributed in production)
    std::map<std::string, ChunkData> chunk_storage_;
    mutable std::mutex storage_mutex_;
    
    // Helper methods
    std::string generate_file_id();
    std::string generate_chunk_id();
    std::vector<std::string> select_nodes_for_chunk(size_t num_nodes);
    std::string calculate_checksum(const std::vector<char>& data);
    std::vector<FileChunk> split_into_chunks(const std::vector<char>& data);
    bool store_chunk(const FileChunk& chunk, const std::vector<char>& data);
    bool retrieve_chunk(const std::string& chunk_id, std::vector<char>& data);
    void cleanup_orphaned_chunks();
    
    // Background tasks
    std::thread heartbeat_monitor_thread_;
    void monitor_heartbeats();
};

// Storage node - stores actual file chunks
class StorageNode {
public:
    StorageNode(const std::string& node_id, const std::string& master_address, 
                int master_port, size_t capacity);
    ~StorageNode();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Chunk operations
    bool store_chunk(const std::string& chunk_id, const std::vector<char>& data);
    bool retrieve_chunk(const std::string& chunk_id, std::vector<char>& data);
    bool delete_chunk(const std::string& chunk_id);
    bool has_chunk(const std::string& chunk_id) const;
    
    // Node info
    std::string get_node_id() const { return node_id_; }
    size_t get_used_space() const;
    size_t get_available_space() const;
    
    // Statistics
    size_t get_chunk_count() const;
    std::vector<std::string> list_chunks() const;
    
private:
    std::string node_id_;
    std::string master_address_;
    int master_port_;
    size_t capacity_;
    std::atomic<bool> running_;
    
    // Chunk storage
    std::map<std::string, std::vector<char>> chunks_;
    mutable std::mutex chunks_mutex_;
    
    // Heartbeat
    std::thread heartbeat_thread_;
    void send_heartbeat();
};

// Client interface for DFS operations
class DFSClient {
public:
    DFSClient(const std::string& master_address, int master_port);
    ~DFSClient();
    
    bool connect();
    void disconnect();
    
    // File operations
    bool upload_file(const std::string& local_path, const std::string& remote_path);
    bool download_file(const std::string& remote_path, const std::string& local_path);
    bool delete_file(const std::string& remote_path);
    bool copy_file(const std::string& source, const std::string& dest);
    bool move_file(const std::string& source, const std::string& dest);
    
    // Directory operations
    std::vector<std::string> list_directory(const std::string& path = "/");
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path, bool recursive = false);
    
    // File info
    FileMetadata get_file_info(const std::string& path);
    bool file_exists(const std::string& path);
    
    // Data operations
    bool write_data(const std::string& remote_path, const std::vector<char>& data);
    bool read_data(const std::string& remote_path, std::vector<char>& data);
    bool append_data(const std::string& remote_path, const std::vector<char>& data);
    
private:
    std::string master_address_;
    int master_port_;
    bool connected_;
    MasterNode* master_;  // Direct reference for now, would be network connection in production
};

// Utility functions
class DFSUtils {
public:
    static std::string calculate_checksum(const std::vector<char>& data);
    static std::vector<char> read_local_file(const std::string& path);
    static bool write_local_file(const std::string& path, const std::vector<char>& data);
    static std::string normalize_path(const std::string& path);
    static std::string get_parent_directory(const std::string& path);
    static std::string get_filename(const std::string& path);
    static bool is_absolute_path(const std::string& path);
};

} // namespace distributed_fs
} // namespace services
