#include "services/distributed_fs/distributed_fs.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>

namespace services {
namespace distributed_fs {

// MasterNode implementation
MasterNode::MasterNode(int port)
    : port_(port)
    , running_(false)
    , chunk_size_(4 * 1024 * 1024)  // 4MB default
    , replication_factor_(3) {
}

MasterNode::~MasterNode() {
    stop();
}

bool MasterNode::start() {
    if (running_) return true;
    
    running_ = true;
    
    // Start heartbeat monitor
    heartbeat_monitor_thread_ = std::thread(&MasterNode::monitor_heartbeats, this);
    
    return true;
}

void MasterNode::stop() {
    if (!running_) return;
    
    running_ = false;
    
    if (heartbeat_monitor_thread_.joinable()) {
        heartbeat_monitor_thread_.join();
    }
}

FileOperationResult MasterNode::create_file(const std::string& path, const std::vector<char>& data) {
    FileOperationResult result;
    
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    // Check if file already exists
    if (file_metadata_.find(path) != file_metadata_.end()) {
        result.message = "File already exists";
        return result;
    }
    
    // Create file metadata
    FileMetadata metadata;
    metadata.file_id = generate_file_id();
    metadata.filename = DFSUtils::get_filename(path);
    metadata.path = path;
    metadata.total_size = data.size();
    metadata.chunk_size = chunk_size_;
    metadata.replication_factor = replication_factor_;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.modified_at = metadata.created_at;
    
    // Split data into chunks
    metadata.chunks = split_into_chunks(data);
    metadata.num_chunks = metadata.chunks.size();
    
    // Store chunks
    size_t offset = 0;
    for (auto& chunk : metadata.chunks) {
        size_t chunk_data_size = std::min(chunk_size_, data.size() - offset);
        std::vector<char> chunk_data(data.begin() + offset, 
                                     data.begin() + offset + chunk_data_size);
        
        if (!store_chunk(chunk, chunk_data)) {
            result.message = "Failed to store chunk " + chunk.chunk_id;
            return result;
        }
        
        offset += chunk_data_size;
    }
    
    // Save metadata
    file_metadata_[path] = metadata;
    
    result.success = true;
    result.file_id = metadata.file_id;
    result.bytes_processed = data.size();
    result.message = "File created successfully";
    
    return result;
}

FileOperationResult MasterNode::read_file(const std::string& path, std::vector<char>& data) {
    FileOperationResult result;
    
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    auto it = file_metadata_.find(path);
    if (it == file_metadata_.end()) {
        result.message = "File not found";
        return result;
    }
    
    const FileMetadata& metadata = it->second;
    data.clear();
    data.reserve(metadata.total_size);
    
    // Retrieve all chunks
    for (const auto& chunk : metadata.chunks) {
        std::vector<char> chunk_data;
        if (!retrieve_chunk(chunk.chunk_id, chunk_data)) {
            result.message = "Failed to retrieve chunk " + chunk.chunk_id;
            return result;
        }
        
        data.insert(data.end(), chunk_data.begin(), chunk_data.end());
    }
    
    result.success = true;
    result.file_id = metadata.file_id;
    result.bytes_processed = data.size();
    result.message = "File read successfully";
    
    return result;
}

FileOperationResult MasterNode::delete_file(const std::string& path) {
    FileOperationResult result;
    
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    auto it = file_metadata_.find(path);
    if (it == file_metadata_.end()) {
        result.message = "File not found";
        return result;
    }
    
    // Delete all chunks
    for (const auto& chunk : it->second.chunks) {
        std::lock_guard<std::mutex> storage_lock(storage_mutex_);
        chunk_storage_.erase(chunk.chunk_id);
    }
    
    // Remove metadata
    file_metadata_.erase(it);
    
    result.success = true;
    result.message = "File deleted successfully";
    
    return result;
}

FileOperationResult MasterNode::update_file(const std::string& path, const std::vector<char>& data) {
    // Delete and recreate
    auto delete_result = delete_file(path);
    if (!delete_result.success) {
        return delete_result;
    }
    
    return create_file(path, data);
}

std::vector<std::string> MasterNode::list_files(const std::string& directory) {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    std::vector<std::string> files;
    std::string normalized_dir = DFSUtils::normalize_path(directory);
    
    for (const auto& [path, metadata] : file_metadata_) {
        if (path.find(normalized_dir) == 0) {
            files.push_back(path);
        }
    }
    
    return files;
}

bool MasterNode::create_directory(const std::string& path) {
    // In a real implementation, would create directory metadata
    return true;
}

bool MasterNode::delete_directory(const std::string& path) {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    std::string normalized = DFSUtils::normalize_path(path);
    std::vector<std::string> to_delete;
    
    // Find all files in directory
    for (const auto& [file_path, metadata] : file_metadata_) {
        if (file_path.find(normalized) == 0) {
            to_delete.push_back(file_path);
        }
    }
    
    // Delete files
    for (const auto& file : to_delete) {
        file_metadata_.erase(file);
    }
    
    return true;
}

FileMetadata* MasterNode::get_file_metadata(const std::string& path) {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    auto it = file_metadata_.find(path);
    if (it != file_metadata_.end()) {
        return &it->second;
    }
    
    return nullptr;
}

std::vector<FileMetadata> MasterNode::get_all_metadata() {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    std::vector<FileMetadata> all_metadata;
    for (const auto& [path, metadata] : file_metadata_) {
        all_metadata.push_back(metadata);
    }
    
    return all_metadata;
}

bool MasterNode::register_node(const StorageNodeInfo& node) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    storage_nodes_[node.node_id] = node;
    return true;
}

bool MasterNode::unregister_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    storage_nodes_.erase(node_id);
    return true;
}

std::vector<StorageNodeInfo> MasterNode::get_active_nodes() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    std::vector<StorageNodeInfo> active;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& [id, node] : storage_nodes_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - node.last_heartbeat).count();
        
        if (elapsed < 30) {  // Node alive if heartbeat within 30 seconds
            active.push_back(node);
        }
    }
    
    return active;
}

void MasterNode::update_node_heartbeat(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = storage_nodes_.find(node_id);
    if (it != storage_nodes_.end()) {
        it->second.last_heartbeat = std::chrono::system_clock::now();
        it->second.is_alive = true;
    }
}

size_t MasterNode::get_total_files() const {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    return file_metadata_.size();
}

size_t MasterNode::get_total_size() const {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    
    size_t total = 0;
    for (const auto& [path, metadata] : file_metadata_) {
        total += metadata.total_size;
    }
    
    return total;
}

size_t MasterNode::get_total_nodes() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    return storage_nodes_.size();
}

std::string MasterNode::generate_file_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "file_";
    for (int i = 0; i < 16; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::string MasterNode::generate_chunk_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "chunk_";
    for (int i = 0; i < 16; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::vector<std::string> MasterNode::select_nodes_for_chunk(size_t num_nodes) {
    std::vector<std::string> selected;
    auto active_nodes = get_active_nodes();
    
    if (active_nodes.size() < num_nodes) {
        num_nodes = active_nodes.size();
    }
    
    // Simple selection: pick nodes with most available space
    std::sort(active_nodes.begin(), active_nodes.end(),
        [](const StorageNodeInfo& a, const StorageNodeInfo& b) {
            return a.get_available_space() > b.get_available_space();
        });
    
    for (size_t i = 0; i < num_nodes && i < active_nodes.size(); i++) {
        selected.push_back(active_nodes[i].node_id);
    }
    
    return selected;
}

std::string MasterNode::calculate_checksum(const std::vector<char>& data) {
    // Simple checksum (in production, use proper hash like SHA256)
    size_t hash = 0;
    for (char c : data) {
        hash = hash * 31 + static_cast<unsigned char>(c);
    }
    
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::vector<FileChunk> MasterNode::split_into_chunks(const std::vector<char>& data) {
    std::vector<FileChunk> chunks;
    
    size_t num_chunks = (data.size() + chunk_size_ - 1) / chunk_size_;
    
    for (size_t i = 0; i < num_chunks; i++) {
        FileChunk chunk;
        chunk.chunk_id = generate_chunk_id();
        chunk.chunk_index = i;
        
        size_t start = i * chunk_size_;
        size_t end = std::min(start + chunk_size_, data.size());
        chunk.size = end - start;
        
        std::vector<char> chunk_data(data.begin() + start, data.begin() + end);
        chunk.checksum = calculate_checksum(chunk_data);
        
        // Select nodes for replicas
        chunk.replica_nodes = select_nodes_for_chunk(replication_factor_);
        
        chunks.push_back(chunk);
    }
    
    return chunks;
}

bool MasterNode::store_chunk(const FileChunk& chunk, const std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    ChunkData chunk_data(chunk.chunk_id, data);
    chunk_data.checksum = chunk.checksum;
    
    chunk_storage_[chunk.chunk_id] = chunk_data;
    
    return true;
}

bool MasterNode::retrieve_chunk(const std::string& chunk_id, std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    auto it = chunk_storage_.find(chunk_id);
    if (it == chunk_storage_.end()) {
        return false;
    }
    
    data = it->second.data;
    return true;
}

void MasterNode::cleanup_orphaned_chunks() {
    // Remove chunks not referenced by any file
    std::lock_guard<std::mutex> storage_lock(storage_mutex_);
    std::lock_guard<std::mutex> metadata_lock(metadata_mutex_);
    
    std::set<std::string> valid_chunks;
    for (const auto& [path, metadata] : file_metadata_) {
        for (const auto& chunk : metadata.chunks) {
            valid_chunks.insert(chunk.chunk_id);
        }
    }
    
    auto it = chunk_storage_.begin();
    while (it != chunk_storage_.end()) {
        if (valid_chunks.find(it->first) == valid_chunks.end()) {
            it = chunk_storage_.erase(it);
        } else {
            ++it;
        }
    }
}

void MasterNode::monitor_heartbeats() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        std::lock_guard<std::mutex> lock(nodes_mutex_);
        auto now = std::chrono::system_clock::now();
        
        for (auto& [id, node] : storage_nodes_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - node.last_heartbeat).count();
            
            node.is_alive = (elapsed < 30);
        }
    }
}

// DFSClient implementation
DFSClient::DFSClient(const std::string& master_address, int master_port)
    : master_address_(master_address)
    , master_port_(master_port)
    , connected_(false)
    , master_(nullptr) {
}

DFSClient::~DFSClient() {
    disconnect();
}

bool DFSClient::connect() {
    // In production, would establish network connection
    // For now, create local master instance
    master_ = new MasterNode(master_port_);
    master_->start();
    connected_ = true;
    return true;
}

void DFSClient::disconnect() {
    if (master_) {
        master_->stop();
        delete master_;
        master_ = nullptr;
    }
    connected_ = false;
}

bool DFSClient::upload_file(const std::string& local_path, const std::string& remote_path) {
    if (!connected_) return false;
    
    auto data = DFSUtils::read_local_file(local_path);
    if (data.empty()) return false;
    
    auto result = master_->create_file(remote_path, data);
    return result.success;
}

bool DFSClient::download_file(const std::string& remote_path, const std::string& local_path) {
    if (!connected_) return false;
    
    std::vector<char> data;
    auto result = master_->read_file(remote_path, data);
    
    if (!result.success) return false;
    
    return DFSUtils::write_local_file(local_path, data);
}

bool DFSClient::delete_file(const std::string& remote_path) {
    if (!connected_) return false;
    
    auto result = master_->delete_file(remote_path);
    return result.success;
}

bool DFSClient::copy_file(const std::string& source, const std::string& dest) {
    if (!connected_) return false;
    
    std::vector<char> data;
    auto read_result = master_->read_file(source, data);
    if (!read_result.success) return false;
    
    auto write_result = master_->create_file(dest, data);
    return write_result.success;
}

bool DFSClient::move_file(const std::string& source, const std::string& dest) {
    if (!copy_file(source, dest)) return false;
    return delete_file(source);
}

std::vector<std::string> DFSClient::list_directory(const std::string& path) {
    if (!connected_) return {};
    
    return master_->list_files(path);
}

bool DFSClient::create_directory(const std::string& path) {
    if (!connected_) return false;
    
    return master_->create_directory(path);
}

bool DFSClient::delete_directory(const std::string& path, bool recursive) {
    if (!connected_) return false;
    
    return master_->delete_directory(path);
}

FileMetadata DFSClient::get_file_info(const std::string& path) {
    if (!connected_) return FileMetadata();
    
    auto* metadata = master_->get_file_metadata(path);
    if (metadata) {
        return *metadata;
    }
    
    return FileMetadata();
}

bool DFSClient::file_exists(const std::string& path) {
    if (!connected_) return false;
    
    return master_->get_file_metadata(path) != nullptr;
}

bool DFSClient::write_data(const std::string& remote_path, const std::vector<char>& data) {
    if (!connected_) return false;
    
    auto result = master_->create_file(remote_path, data);
    return result.success;
}

bool DFSClient::read_data(const std::string& remote_path, std::vector<char>& data) {
    if (!connected_) return false;
    
    auto result = master_->read_file(remote_path, data);
    return result.success;
}

bool DFSClient::append_data(const std::string& remote_path, const std::vector<char>& data) {
    if (!connected_) return false;
    
    std::vector<char> existing_data;
    auto read_result = master_->read_file(remote_path, existing_data);
    
    if (read_result.success) {
        existing_data.insert(existing_data.end(), data.begin(), data.end());
        auto write_result = master_->update_file(remote_path, existing_data);
        return write_result.success;
    }
    
    return false;
}

// DFSUtils implementation
std::string DFSUtils::calculate_checksum(const std::vector<char>& data) {
    size_t hash = 0;
    for (char c : data) {
        hash = hash * 31 + static_cast<unsigned char>(c);
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::vector<char> DFSUtils::read_local_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> data(size);
    file.read(data.data(), size);
    
    return data;
}

bool DFSUtils::write_local_file(const std::string& path, const std::vector<char>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.write(data.data(), data.size());
    return true;
}

std::string DFSUtils::normalize_path(const std::string& path) {
    std::string normalized = path;
    
    // Ensure starts with /
    if (normalized.empty() || normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Remove trailing /
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized;
}

std::string DFSUtils::get_parent_directory(const std::string& path) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos || last_slash == 0) {
        return "/";
    }
    
    return path.substr(0, last_slash);
}

std::string DFSUtils::get_filename(const std::string& path) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        return path;
    }
    
    return path.substr(last_slash + 1);
}

bool DFSUtils::is_absolute_path(const std::string& path) {
    return !path.empty() && path[0] == '/';
}

} // namespace distributed_fs
} // namespace services
