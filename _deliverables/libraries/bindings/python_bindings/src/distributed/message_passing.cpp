#include "distributed/message_passing.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace distributed {

struct DistributedContext::SharedBus {
    explicit SharedBus(int size) : world_size(size) {}

    int world_size;
    std::map<int, std::deque<Message>> queues;
    std::mutex mutex;
    std::condition_variable condition;
    int barrier_count = 0;
    std::size_t barrier_generation = 0;
};

std::shared_ptr<DistributedContext::SharedBus>
DistributedContext::acquire_bus(
    const std::string& cluster_id, int world_size) {
    static std::mutex registry_mutex;
    static std::unordered_map<
        std::string,
        std::weak_ptr<SharedBus>> registry;

    std::lock_guard<std::mutex> lock(registry_mutex);
    const std::string key =
        cluster_id + ":" + std::to_string(world_size);
    std::shared_ptr<SharedBus> bus = registry[key].lock();
    if (!bus) {
        bus = std::make_shared<SharedBus>(world_size);
        registry[key] = bus;
    }
    return bus;
}

// DistributedContext implementation
DistributedContext::DistributedContext(
    int world_size, int rank, std::string cluster_id)
    : world_size_(world_size),
      rank_(rank),
      cluster_id_(std::move(cluster_id)) {
    if (world_size_ <= 0) {
        throw std::invalid_argument("World size must be positive");
    }
    if (rank_ < 0 || rank_ >= world_size_) {
        throw std::invalid_argument("Invalid rank");
    }
    if (cluster_id_.empty()) {
        throw std::invalid_argument("Cluster id cannot be empty");
    }
    bus_ = acquire_bus(cluster_id_, world_size_);
}

DistributedContext::~DistributedContext() = default;

void DistributedContext::send(const Message& msg, int dest_rank) {
    if (dest_rank < 0 || dest_rank >= world_size_) {
        throw std::invalid_argument("Invalid destination rank");
    }
    Message copy = msg;
    if (copy.source_rank < 0) {
        copy.source_rank = rank_;
    }
    copy.dest_rank = dest_rank;
    {
        std::lock_guard<std::mutex> lock(bus_->mutex);
        bus_->queues[dest_rank].push_back(std::move(copy));
    }
    bus_->condition.notify_all();
}

Message DistributedContext::receive(int source_rank) {
    Message message;
    std::unique_lock<std::mutex> lock(bus_->mutex);
    auto ready = [&]() {
        const auto queue = bus_->queues.find(rank_);
        if (queue == bus_->queues.end()) {
            return false;
        }
        return std::any_of(
            queue->second.begin(),
            queue->second.end(),
            [&](const Message& candidate) {
                return source_rank < 0 ||
                       candidate.source_rank == source_rank;
            });
    };
    bus_->condition.wait(lock, ready);
    auto& queue = bus_->queues[rank_];
    const auto iterator = std::find_if(
        queue.begin(),
        queue.end(),
        [&](const Message& candidate) {
            return source_rank < 0 ||
                   candidate.source_rank == source_rank;
        });
    message = std::move(*iterator);
    queue.erase(iterator);
    return message;
}

bool DistributedContext::receive_for(
    Message& message, int timeout_ms, int source_rank) {
    if (timeout_ms < 0) {
        throw std::invalid_argument("Timeout cannot be negative");
    }
    std::unique_lock<std::mutex> lock(bus_->mutex);
    auto ready = [&]() {
        const auto queue = bus_->queues.find(rank_);
        if (queue == bus_->queues.end()) {
            return false;
        }
        return std::any_of(
            queue->second.begin(),
            queue->second.end(),
            [&](const Message& candidate) {
                return source_rank < 0 ||
                       candidate.source_rank == source_rank;
            });
    };
    if (!bus_->condition.wait_for(
            lock, std::chrono::milliseconds(timeout_ms), ready)) {
        return false;
    }
    auto& queue = bus_->queues[rank_];
    const auto iterator = std::find_if(
        queue.begin(),
        queue.end(),
        [&](const Message& candidate) {
            return source_rank < 0 ||
                   candidate.source_rank == source_rank;
        });
    message = std::move(*iterator);
    queue.erase(iterator);
    return true;
}

void DistributedContext::broadcast(std::vector<double>& data, int root_rank) {
    if (rank_ == root_rank) {
        // Send to all other ranks
        for (int r = 0; r < world_size_; ++r) {
            if (r != root_rank) {
                Message msg(MessageType::DATA, rank_, r);
                msg.data = data;
                send(msg, r);
            }
        }
    } else {
        // Receive from root
        Message msg = receive(root_rank);
        data = msg.data;
    }
}

void DistributedContext::scatter(const std::vector<double>& send_data,
                                 std::vector<double>& recv_data,
                                 int root_rank) {
    size_t chunk_size = send_data.size() / world_size_;
    
    if (rank_ == root_rank) {
        // Send chunks to all workers
        for (int r = 0; r < world_size_; ++r) {
            size_t start = r * chunk_size;
            size_t end = (r == world_size_ - 1) ? send_data.size() : (r + 1) * chunk_size;
            
            if (r == root_rank) {
                recv_data.assign(send_data.begin() + start, send_data.begin() + end);
            } else {
                Message msg(MessageType::DATA, rank_, r);
                msg.data.assign(send_data.begin() + start, send_data.begin() + end);
                send(msg, r);
            }
        }
    } else {
        // Receive chunk
        Message msg = receive(root_rank);
        recv_data = msg.data;
    }
}

void DistributedContext::gather(const std::vector<double>& send_data,
                                std::vector<double>& recv_data,
                                int root_rank) {
    if (rank_ == root_rank) {
        // Collect from all workers
        std::vector<std::vector<double>> gathered(world_size_);
        gathered[root_rank] = send_data;
        
        for (int r = 0; r < world_size_; ++r) {
            if (r != root_rank) {
                Message msg = receive(r);
                gathered[r] = msg.data;
            }
        }
        
        // Concatenate all data
        recv_data.clear();
        for (const auto& chunk : gathered) {
            recv_data.insert(recv_data.end(), chunk.begin(), chunk.end());
        }
    } else {
        // Send to root
        Message msg(MessageType::DATA, rank_, root_rank);
        msg.data = send_data;
        send(msg, root_rank);
    }
}

std::vector<double> DistributedContext::all_reduce(const std::vector<double>& data,
                                                   ReduceOp op) {
    // Simple tree-based all-reduce
    std::vector<std::vector<double>> all_data(world_size_);
    
    // Gather to root
    std::vector<double> gathered;
    if (rank_ == 0) {
        all_data[0] = data;
        for (int r = 1; r < world_size_; ++r) {
            Message msg = receive(r);
            all_data[r] = msg.data;
        }
        gathered = apply_reduce_op(all_data, op);
    } else {
        Message msg(MessageType::DATA, rank_, 0);
        msg.data = data;
        send(msg, 0);
    }
    
    // Broadcast result
    broadcast(gathered, 0);
    return gathered;
}

std::vector<double> DistributedContext::ring_all_reduce(const std::vector<double>& data) {
    // Ring all-reduce: more efficient for large data
    std::vector<double> result = data;
    size_t chunk_size = data.size() / world_size_;
    
    // Ring reduce-scatter
    for (int step = 0; step < world_size_ - 1; ++step) {
        int send_rank = (rank_ + 1) % world_size_;
        int recv_rank = (rank_ - 1 + world_size_) % world_size_;
        
        int chunk_idx = (rank_ - step + world_size_) % world_size_;
        size_t start = chunk_idx * chunk_size;
        size_t end = (chunk_idx == world_size_ - 1) ? data.size() : (chunk_idx + 1) * chunk_size;
        
        // Send chunk
        Message send_msg(MessageType::DATA, rank_, send_rank);
        send_msg.data.assign(result.begin() + start, result.begin() + end);
        send(send_msg, send_rank);
        
        // Receive and accumulate
        Message recv_msg = receive(recv_rank);
        for (size_t i = 0; i < recv_msg.data.size(); ++i) {
            result[start + i] += recv_msg.data[i];
        }
    }
    
    // Ring all-gather
    for (int step = 0; step < world_size_ - 1; ++step) {
        int send_rank = (rank_ + 1) % world_size_;
        int recv_rank = (rank_ - 1 + world_size_) % world_size_;
        
        int chunk_idx = (rank_ - step + 1 + world_size_) % world_size_;
        size_t start = chunk_idx * chunk_size;
        size_t end = (chunk_idx == world_size_ - 1) ? data.size() : (chunk_idx + 1) * chunk_size;
        
        // Send chunk
        Message send_msg(MessageType::DATA, rank_, send_rank);
        send_msg.data.assign(result.begin() + start, result.begin() + end);
        send(send_msg, send_rank);
        
        // Receive
        Message recv_msg = receive(recv_rank);
        std::copy(recv_msg.data.begin(), recv_msg.data.end(), result.begin() + start);
    }
    
    return result;
}

void DistributedContext::barrier() {
    std::unique_lock<std::mutex> lock(bus_->mutex);
    const std::size_t generation = bus_->barrier_generation;
    ++bus_->barrier_count;

    if (bus_->barrier_count >= world_size_) {
        bus_->barrier_count = 0;
        ++bus_->barrier_generation;
        bus_->condition.notify_all();
    } else {
        bus_->condition.wait(lock, [&] {
            return bus_->barrier_generation != generation;
        });
    }
}

std::vector<double> DistributedContext::apply_reduce_op(
    const std::vector<std::vector<double>>& data, ReduceOp op) {
    
    if (data.empty()) return {};
    
    size_t size = data[0].size();
    std::vector<double> result(size);
    
    switch (op) {
        case ReduceOp::SUM:
            for (const auto& vec : data) {
                for (size_t i = 0; i < size; ++i) {
                    result[i] += vec[i];
                }
            }
            break;
            
        case ReduceOp::AVERAGE:
            for (const auto& vec : data) {
                for (size_t i = 0; i < size; ++i) {
                    result[i] += vec[i];
                }
            }
            for (double& val : result) {
                val /= data.size();
            }
            break;
            
        case ReduceOp::MIN:
            result = data[0];
            for (size_t i = 1; i < data.size(); ++i) {
                for (size_t j = 0; j < size; ++j) {
                    result[j] = std::min(result[j], data[i][j]);
                }
            }
            break;
            
        case ReduceOp::MAX:
            result = data[0];
            for (size_t i = 1; i < data.size(); ++i) {
                for (size_t j = 0; j < size; ++j) {
                    result[j] = std::max(result[j], data[i][j]);
                }
            }
            break;
            
        case ReduceOp::PRODUCT:
            std::fill(result.begin(), result.end(), 1.0);
            for (const auto& vec : data) {
                for (size_t i = 0; i < size; ++i) {
                    result[i] *= vec[i];
                }
            }
            break;
    }
    
    return result;
}

// DataPartitioner implementation
DataPartitioner::DataPartitioner(size_t total_size, int world_size)
    : total_size_(total_size), world_size_(world_size) {
    
    size_t base_size = total_size / world_size;
    size_t remainder = total_size % world_size;
    
    size_t current = 0;
    for (size_t i = 0; i < static_cast<size_t>(world_size); ++i) {
        size_t size = base_size + (i < remainder ? 1 : 0);
        partitions_.emplace_back(current, current + size);
        current += size;
    }
}

std::pair<size_t, size_t> DataPartitioner::get_partition(int rank) const {
    if (rank < 0 || rank >= world_size_) {
        throw std::invalid_argument("Invalid rank");
    }
    return partitions_[rank];
}

std::vector<size_t> DataPartitioner::get_indices(int rank) const {
    auto [start, end] = get_partition(rank);
    std::vector<size_t> indices(end - start);
    std::iota(indices.begin(), indices.end(), start);
    return indices;
}

size_t DataPartitioner::partition_size(int rank) const {
    auto [start, end] = get_partition(rank);
    return end - start;
}

// ParameterServer implementation
ParameterServer::ParameterServer(int world_size)
    : world_size_(world_size) {}

void ParameterServer::set_parameters(const std::string& key,
                                     const std::vector<double>& params) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    parameters_[key] = params;
}

std::vector<double> ParameterServer::get_parameters(const std::string& key) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    return parameters_[key];
}

void ParameterServer::update_parameters(const std::string& key,
                                        const std::vector<double>& gradients,
                                        double learning_rate) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    auto& params = parameters_[key];
    
    if (params.size() != gradients.size()) {
        params.resize(gradients.size());
    }
    
    for (size_t i = 0; i < params.size(); ++i) {
        params[i] -= learning_rate * gradients[i];
    }
}

void ParameterServer::accumulate_gradient(const std::string& key,
                                         const std::vector<double>& gradient,
                                         int worker_rank) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    if (gradient_buffer_[key].size() < static_cast<size_t>(world_size_)) {
        gradient_buffer_[key].resize(world_size_);
    }
    
    gradient_buffer_[key][worker_rank] = gradient;
}

void ParameterServer::apply_gradients(const std::string& key, double learning_rate) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    auto& grads = gradient_buffer_[key];
    if (grads.empty()) return;
    
    // Average gradients
    size_t grad_size = grads[0].size();
    std::vector<double> avg_gradient(grad_size, 0.0);
    
    for (const auto& grad : grads) {
        for (size_t i = 0; i < grad_size; ++i) {
            avg_gradient[i] += grad[i];
        }
    }
    
    for (double& val : avg_gradient) {
        val /= grads.size();
    }
    
    // Update parameters
    auto& params = parameters_[key];
    for (size_t i = 0; i < params.size(); ++i) {
        params[i] -= learning_rate * avg_gradient[i];
    }
}

void ParameterServer::clear_gradients(const std::string& key) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    gradient_buffer_[key].clear();
}

} // namespace distributed
