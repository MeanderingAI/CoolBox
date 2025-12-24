#include "networking/distributed/distributed_trainer.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

namespace networking {
namespace distributed {

// DistributedTrainer base implementation
DistributedTrainer::DistributedTrainer(std::shared_ptr<DistributedContext> context,
                                       TrainingStrategy strategy)
    : context_(context), strategy_(strategy) {}

void DistributedTrainer::synchronize_model() {
    auto params = get_parameters();
    context_->broadcast(params, 0);
    if (context_->rank() != 0) {
        set_parameters(params);
    }
}

std::vector<double> DistributedTrainer::aggregate_gradients(
    const std::vector<double>& local_gradient,
    AggregationMethod method) {
    
    switch (method) {
        case AggregationMethod::SYNCHRONOUS:
            return context_->all_reduce(local_gradient, ReduceOp::AVERAGE);
            
        case AggregationMethod::ASYNCHRONOUS:
            // For async, just return local gradient
            // (coordination handled by parameter server)
            return local_gradient;
            
        case AggregationMethod::ELASTIC_AVERAGING: {
            // Elastic averaging: local update + pull from average
            auto global_avg = context_->all_reduce(local_gradient, ReduceOp::AVERAGE);
            std::vector<double> elastic(local_gradient.size());
            double alpha = 0.5; // Elasticity parameter
            for (size_t i = 0; i < elastic.size(); ++i) {
                elastic[i] = alpha * local_gradient[i] + (1 - alpha) * global_avg[i];
            }
            return elastic;
        }
    }
    
    return local_gradient;
}

// DistributedNeuralNetTrainer implementation
DistributedNeuralNetTrainer::DistributedNeuralNetTrainer(
    std::shared_ptr<DistributedContext> context,
    TrainingStrategy strategy,
    int input_dim,
    std::vector<int> hidden_dims,
    int output_dim,
    double learning_rate)
    : DistributedTrainer(context, strategy),
      learning_rate_(learning_rate),
      local_loss_(0.0) {
    
    // Initialize network architecture
    std::vector<int> layer_sizes = {input_dim};
    layer_sizes.insert(layer_sizes.end(), hidden_dims.begin(), hidden_dims.end());
    layer_sizes.push_back(output_dim);
    
    // Initialize weights and biases
    std::random_device rd;
    std::mt19937 gen(rd() + context->rank());
    std::normal_distribution<double> dist(0.0, 0.1);
    
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        int in_size = layer_sizes[i];
        int out_size = layer_sizes[i + 1];
        
        std::vector<double> w(in_size * out_size);
        for (double& val : w) {
            val = dist(gen);
        }
        weights_.push_back(w);
        
        std::vector<double> b(out_size, 0.0);
        biases_.push_back(b);
    }
}

void DistributedNeuralNetTrainer::train_epoch(
    const std::vector<std::vector<double>>& local_data,
    const std::vector<std::vector<double>>& local_labels) {
    
    // Compute gradients on local data
    std::vector<std::vector<double>> weight_grads(weights_.size());
    std::vector<std::vector<double>> bias_grads(biases_.size());
    
    for (size_t i = 0; i < weights_.size(); ++i) {
        weight_grads[i].resize(weights_[i].size(), 0.0);
        bias_grads[i].resize(biases_[i].size(), 0.0);
    }
    
    local_loss_ = 0.0;
    
    // Mini-batch gradient descent on local data
    for (size_t i = 0; i < local_data.size(); ++i) {
        auto pred = forward(local_data[i]);
        
        // Compute loss
        for (size_t j = 0; j < pred.size(); ++j) {
            double diff = pred[j] - local_labels[i][j];
            local_loss_ += diff * diff;
        }
        
        // Backprop (simplified - accumulate gradients)
        auto grad = compute_gradient(local_data[i], local_labels[i]);
        
        // Accumulate gradients (simplified)
        for (size_t j = 0; j < weight_grads[0].size(); ++j) {
            weight_grads[0][j] += grad[j];
        }
    }
    
    local_loss_ /= local_data.size();
    
    // Average local gradients
    for (auto& wg : weight_grads) {
        for (double& val : wg) {
            val /= local_data.size();
        }
    }
    
    // Aggregate gradients across workers
    for (size_t i = 0; i < weights_.size(); ++i) {
        auto aggregated_w = aggregate_gradients(weight_grads[i]);
        auto aggregated_b = aggregate_gradients(bias_grads[i]);
        
        // Update weights
        for (size_t j = 0; j < weights_[i].size(); ++j) {
            weights_[i][j] -= learning_rate_ * aggregated_w[j];
        }
        for (size_t j = 0; j < biases_[i].size(); ++j) {
            biases_[i][j] -= learning_rate_ * aggregated_b[j];
        }
    }
}

std::vector<double> DistributedNeuralNetTrainer::forward(const std::vector<double>& input) {
    std::vector<double> activation = input;
    
    for (size_t layer = 0; layer < weights_.size(); ++layer) {
        std::vector<double> next_activation(biases_[layer].size());
        
        // Matrix multiplication + bias
        for (size_t i = 0; i < biases_[layer].size(); ++i) {
            double sum = biases_[layer][i];
            for (size_t j = 0; j < activation.size(); ++j) {
                sum += weights_[layer][j * biases_[layer].size() + i] * activation[j];
            }
            // ReLU activation (except last layer)
            next_activation[i] = (layer < weights_.size() - 1) ? std::max(0.0, sum) : sum;
        }
        
        activation = next_activation;
    }
    
    return activation;
}

std::vector<double> DistributedNeuralNetTrainer::compute_gradient(
    const std::vector<double>& input,
    const std::vector<double>& target) {
    
    // Simplified gradient computation (for demonstration)
    auto pred = forward(input);
    std::vector<double> grad(weights_[0].size());
    
    // Output gradient
    for (size_t i = 0; i < pred.size(); ++i) {
        double delta = 2.0 * (pred[i] - target[i]);
        
        // Backprop to first layer (simplified)
        for (size_t j = 0; j < input.size(); ++j) {
            if (j < grad.size()) {
                grad[j] += delta * input[j];
            }
        }
    }
    
    return grad;
}

std::vector<double> DistributedNeuralNetTrainer::get_parameters() const {
    std::vector<double> params;
    for (const auto& w : weights_) {
        params.insert(params.end(), w.begin(), w.end());
    }
    for (const auto& b : biases_) {
        params.insert(params.end(), b.begin(), b.end());
    }
    return params;
}

void DistributedNeuralNetTrainer::set_parameters(const std::vector<double>& params) {
    size_t offset = 0;
    for (auto& w : weights_) {
        std::copy(params.begin() + offset, params.begin() + offset + w.size(), w.begin());
        offset += w.size();
    }
    for (auto& b : biases_) {
        std::copy(params.begin() + offset, params.begin() + offset + b.size(), b.begin());
        offset += b.size();
    }
}

std::vector<double> DistributedNeuralNetTrainer::predict(const std::vector<double>& input) {
    return forward(input);
}

double DistributedNeuralNetTrainer::get_global_loss() {
    std::vector<double> loss_vec = {local_loss_};
    auto global_loss_vec = context_->all_reduce(loss_vec, ReduceOp::AVERAGE);
    return global_loss_vec[0];
}

// DistributedKMeansTrainer implementation
DistributedKMeansTrainer::DistributedKMeansTrainer(
    std::shared_ptr<DistributedContext> context,
    int n_clusters,
    int max_iterations)
    : DistributedTrainer(context, TrainingStrategy::DATA_PARALLEL),
      n_clusters_(n_clusters),
      max_iterations_(max_iterations) {}

void DistributedKMeansTrainer::train_epoch(
    const std::vector<std::vector<double>>& local_data,
    const std::vector<std::vector<double>>& local_labels) {
    
    if (centroids_.empty() && context_->rank() == 0) {
        initialize_centroids(local_data);
    }
    
    // Broadcast centroids
    std::vector<double> flat_centroids;
    if (context_->rank() == 0) {
        for (const auto& centroid : centroids_) {
            flat_centroids.insert(flat_centroids.end(), centroid.begin(), centroid.end());
        }
    }
    context_->broadcast(flat_centroids, 0);
    
    // Reconstruct centroids
    if (context_->rank() != 0) {
        size_t dim = local_data[0].size();
        centroids_.clear();
        for (size_t i = 0; i < flat_centroids.size(); i += dim) {
            centroids_.emplace_back(flat_centroids.begin() + i,
                                   flat_centroids.begin() + i + dim);
        }
    }
    
    // Assign points to clusters (local)
    std::vector<std::vector<std::vector<double>>> local_clusters(n_clusters_);
    for (const auto& point : local_data) {
        int cluster = find_nearest_centroid(point);
        local_clusters[cluster].push_back(point);
    }
    
    // Compute local centroids
    std::vector<std::vector<double>> local_centroids(n_clusters_);
    for (int k = 0; k < n_clusters_; ++k) {
        if (local_clusters[k].empty()) {
            local_centroids[k] = centroids_[k]; // Keep old centroid
        } else {
            size_t dim = local_data[0].size();
            local_centroids[k].resize(dim, 0.0);
            
            for (const auto& point : local_clusters[k]) {
                for (size_t d = 0; d < dim; ++d) {
                    local_centroids[k][d] += point[d];
                }
            }
            
            for (double& val : local_centroids[k]) {
                val /= local_clusters[k].size();
            }
        }
    }
    
    // Aggregate centroids across workers
    std::vector<double> flat_local;
    for (const auto& centroid : local_centroids) {
        flat_local.insert(flat_local.end(), centroid.begin(), centroid.end());
    }
    
    auto aggregated = context_->all_reduce(flat_local, ReduceOp::AVERAGE);
    
    // Update centroids
    centroids_.clear();
    size_t dim = local_data[0].size();
    for (size_t i = 0; i < aggregated.size(); i += dim) {
        centroids_.emplace_back(aggregated.begin() + i, aggregated.begin() + i + dim);
    }
}

void DistributedKMeansTrainer::initialize_centroids(
    const std::vector<std::vector<double>>& data) {
    
    // K-means++ initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, data.size() - 1);
    
    centroids_.clear();
    centroids_.push_back(data[dis(gen)]);
    
    for (int k = 1; k < n_clusters_; ++k) {
        std::vector<double> distances(data.size());
        
        for (size_t i = 0; i < data.size(); ++i) {
            double min_dist = std::numeric_limits<double>::max();
            for (const auto& centroid : centroids_) {
                double dist = 0.0;
                for (size_t d = 0; d < data[i].size(); ++d) {
                    double diff = data[i][d] - centroid[d];
                    dist += diff * diff;
                }
                min_dist = std::min(min_dist, dist);
            }
            distances[i] = min_dist;
        }
        
        std::discrete_distribution<> weighted_dis(distances.begin(), distances.end());
        centroids_.push_back(data[weighted_dis(gen)]);
    }
}

int DistributedKMeansTrainer::find_nearest_centroid(const std::vector<double>& point) {
    int nearest = 0;
    double min_dist = std::numeric_limits<double>::max();
    
    for (size_t k = 0; k < centroids_.size(); ++k) {
        double dist = 0.0;
        for (size_t d = 0; d < point.size(); ++d) {
            double diff = point[d] - centroids_[k][d];
            dist += diff * diff;
        }
        
        if (dist < min_dist) {
            min_dist = dist;
            nearest = k;
        }
    }
    
    return nearest;
}

std::vector<double> DistributedKMeansTrainer::get_parameters() const {
    std::vector<double> params;
    for (const auto& centroid : centroids_) {
        params.insert(params.end(), centroid.begin(), centroid.end());
    }
    return params;
}

void DistributedKMeansTrainer::set_parameters(const std::vector<double>& params) {
    if (centroids_.empty()) return;
    
    size_t dim = centroids_[0].size();
    centroids_.clear();
    
    for (size_t i = 0; i < params.size(); i += dim) {
        centroids_.emplace_back(params.begin() + i, params.begin() + i + dim);
    }
}

std::vector<double> DistributedKMeansTrainer::predict(const std::vector<double>& input) {
    int cluster = find_nearest_centroid(input);
    return {static_cast<double>(cluster)};
}

// Utility functions
namespace utils {

std::vector<std::vector<std::vector<double>>>
partition_data(const std::vector<std::vector<double>>& data, int world_size) {
    DataPartitioner partitioner(data.size(), world_size);
    std::vector<std::vector<std::vector<double>>> partitions(world_size);
    
    for (int rank = 0; rank < world_size; ++rank) {
        auto indices = partitioner.get_indices(rank);
        for (size_t idx : indices) {
            partitions[rank].push_back(data[idx]);
        }
    }
    
    return partitions;
}

double compute_distributed_accuracy(
    const std::vector<std::vector<double>>& predictions,
    const std::vector<std::vector<double>>& labels,
    std::shared_ptr<DistributedContext> context) {
    
    int local_correct = 0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        bool correct = true;
        for (size_t j = 0; j < predictions[i].size(); ++j) {
            if (std::abs(predictions[i][j] - labels[i][j]) > 0.5) {
                correct = false;
                break;
            }
        }
        if (correct) ++local_correct;
    }
    
    std::vector<double> correct_vec = {static_cast<double>(local_correct)};
    std::vector<double> total_vec = {static_cast<double>(predictions.size())};
    
    auto global_correct = context->all_reduce(correct_vec, ReduceOp::SUM);
    auto global_total = context->all_reduce(total_vec, ReduceOp::SUM);
    
    return global_correct[0] / global_total[0];
}

} // namespace utils

} // namespace distributed
} // namespace networking
