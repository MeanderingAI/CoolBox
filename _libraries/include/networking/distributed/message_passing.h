#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace networking {
namespace distributed {

// Message types for distributed communication
enum class MessageType {
    DATA,           // Training data
    GRADIENT,       // Gradient updates
    PARAMETER,      // Model parameters
    COMMAND,        // Control commands
    RESULT,         // Computation results
    HEARTBEAT,      // Worker health check
    BARRIER,        // Synchronization barrier
    REDUCE          // Reduction operation
};

// Communication patterns
enum class CommPattern {
    POINT_TO_POINT,     // Direct worker-to-worker
    BROADCAST,          // One-to-all
    SCATTER,            // Distribute chunks to workers
    GATHER,             // Collect from all workers
    ALL_REDUCE,         // Reduce and broadcast result
    RING_ALL_REDUCE     // Ring-based all-reduce (efficient)
};

// Reduction operations
enum class ReduceOp {
    SUM,
    AVERAGE,
    MIN,
    MAX,
    PRODUCT
};

// Message container
struct Message {
    MessageType type;
    int source_rank;
    int dest_rank;
    std::vector<double> data;
    std::map<std::string, std::string> metadata;
    
    Message(MessageType t = MessageType::DATA, int src = -1, int dst = -1)
        : type(t), source_rank(src), dest_rank(dst) {}
};

// Distributed context - manages communication
class DistributedContext {
public:
    DistributedContext(int world_size, int rank);
    ~DistributedContext();
    
    // Basic communication
    void send(const Message& msg, int dest_rank);
    Message receive(int source_rank = -1); // -1 means any source
    
    // Collective operations
    void broadcast(std::vector<double>& data, int root_rank);
    void scatter(const std::vector<double>& send_data, 
                 std::vector<double>& recv_data, int root_rank);
    void gather(const std::vector<double>& send_data,
                std::vector<double>& recv_data, int root_rank);
    std::vector<double> all_reduce(const std::vector<double>& data, ReduceOp op);
    std::vector<double> ring_all_reduce(const std::vector<double>& data);
    
    // Synchronization
    void barrier();
    
    // Properties
    int rank() const { return rank_; }
    int world_size() const { return world_size_; }
    bool is_master() const { return rank_ == 0; }
    
private:
    int world_size_;
    int rank_;
    
    // Simulated message queue (in real implementation, use MPI/gRPC/ZeroMQ)
    std::map<int, std::queue<Message>> message_queues_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::vector<double> apply_reduce_op(const std::vector<std::vector<double>>& data,
                                        ReduceOp op);
};

// Data partitioner for distributing datasets
class DataPartitioner {
public:
    DataPartitioner(size_t total_size, int world_size);
    
    // Get partition for specific rank
    std::pair<size_t, size_t> get_partition(int rank) const;
    
    // Get indices for a rank
    std::vector<size_t> get_indices(int rank) const;
    
    size_t partition_size(int rank) const;
    
private:
    size_t total_size_;
    int world_size_;
    std::vector<std::pair<size_t, size_t>> partitions_;
};

// Parameter server for centralized parameter management
class ParameterServer {
public:
    ParameterServer(int world_size);
    
    // Store parameters by key
    void set_parameters(const std::string& key, const std::vector<double>& params);
    std::vector<double> get_parameters(const std::string& key);
    
    // Update parameters (for gradient descent)
    void update_parameters(const std::string& key, 
                          const std::vector<double>& gradients,
                          double learning_rate);
    
    // Aggregate gradients from workers
    void accumulate_gradient(const std::string& key,
                            const std::vector<double>& gradient,
                            int worker_rank);
    
    // Apply accumulated gradients
    void apply_gradients(const std::string& key, double learning_rate);
    
    void clear_gradients(const std::string& key);
    
private:
    int world_size_;
    std::map<std::string, std::vector<double>> parameters_;
    std::map<std::string, std::vector<std::vector<double>>> gradient_buffer_;
    std::mutex param_mutex_;
};

} // namespace distributed
} // namespace networking
