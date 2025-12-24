#pragma once

#include "message_passing.h"
#include <vector>
#include <memory>
#include <functional>

namespace networking {
namespace distributed {

// Strategy for distributed training
enum class TrainingStrategy {
    DATA_PARALLEL,          // Each worker has full model, different data
    MODEL_PARALLEL,         // Model split across workers
    PARAMETER_SERVER,       // Centralized parameter management
    DECENTRALIZED,          // Peer-to-peer gradient exchange
    FEDERATED              // Federated learning (local training + aggregation)
};

// Gradient aggregation method
enum class AggregationMethod {
    SYNCHRONOUS,           // Wait for all workers
    ASYNCHRONOUS,          // Update as gradients arrive
    ELASTIC_AVERAGING      // Elastic averaging SGD
};

// Base distributed trainer
class DistributedTrainer {
public:
    DistributedTrainer(std::shared_ptr<DistributedContext> context,
                       TrainingStrategy strategy);
    virtual ~DistributedTrainer() = default;
    
    // Training interface
    virtual void train_epoch(const std::vector<std::vector<double>>& local_data,
                            const std::vector<std::vector<double>>& local_labels) = 0;
    
    // Get local model parameters
    virtual std::vector<double> get_parameters() const = 0;
    
    // Set model parameters
    virtual void set_parameters(const std::vector<double>& params) = 0;
    
    // Prediction
    virtual std::vector<double> predict(const std::vector<double>& input) = 0;
    
    // Synchronize model across workers
    void synchronize_model();
    
    // Aggregate gradients
    std::vector<double> aggregate_gradients(const std::vector<double>& local_gradient,
                                           AggregationMethod method = AggregationMethod::SYNCHRONOUS);
    
protected:
    std::shared_ptr<DistributedContext> context_;
    TrainingStrategy strategy_;
};

// Distributed neural network trainer
class DistributedNeuralNetTrainer : public DistributedTrainer {
public:
    DistributedNeuralNetTrainer(std::shared_ptr<DistributedContext> context,
                                TrainingStrategy strategy,
                                int input_dim, 
                                std::vector<int> hidden_dims,
                                int output_dim,
                                double learning_rate);
    
    void train_epoch(const std::vector<std::vector<double>>& local_data,
                    const std::vector<std::vector<double>>& local_labels) override;
    
    std::vector<double> get_parameters() const override;
    void set_parameters(const std::vector<double>& params) override;
    std::vector<double> predict(const std::vector<double>& input) override;
    
    // Training statistics
    double get_local_loss() const { return local_loss_; }
    double get_global_loss();
    
private:
    std::vector<std::vector<double>> weights_;
    std::vector<std::vector<double>> biases_;
    double learning_rate_;
    double local_loss_;
    
    std::vector<double> forward(const std::vector<double>& input);
    std::vector<double> compute_gradient(const std::vector<double>& input,
                                         const std::vector<double>& target);
};

// Distributed random forest trainer
class DistributedRandomForestTrainer : public DistributedTrainer {
public:
    DistributedRandomForestTrainer(std::shared_ptr<DistributedContext> context,
                                   int n_trees,
                                   int max_depth,
                                   int min_samples_split);
    
    void train_epoch(const std::vector<std::vector<double>>& local_data,
                    const std::vector<std::vector<double>>& local_labels) override;
    
    std::vector<double> get_parameters() const override;
    void set_parameters(const std::vector<double>& params) override;
    std::vector<double> predict(const std::vector<double>& input) override;
    
private:
    int n_trees_;
    int max_depth_;
    int min_samples_split_;
    std::vector<std::vector<double>> tree_parameters_; // Serialized trees
};

// Distributed K-Means trainer
class DistributedKMeansTrainer : public DistributedTrainer {
public:
    DistributedKMeansTrainer(std::shared_ptr<DistributedContext> context,
                            int n_clusters,
                            int max_iterations);
    
    void train_epoch(const std::vector<std::vector<double>>& local_data,
                    const std::vector<std::vector<double>>& local_labels) override;
    
    std::vector<double> get_parameters() const override;
    void set_parameters(const std::vector<double>& params) override;
    std::vector<double> predict(const std::vector<double>& input) override;
    
    std::vector<std::vector<double>> get_centroids() const { return centroids_; }
    
private:
    int n_clusters_;
    int max_iterations_;
    std::vector<std::vector<double>> centroids_;
    
    void initialize_centroids(const std::vector<std::vector<double>>& data);
    int find_nearest_centroid(const std::vector<double>& point);
};

// Coordinator for managing distributed training
class TrainingCoordinator {
public:
    TrainingCoordinator(int world_size);
    
    // Register a trainer
    void register_trainer(std::shared_ptr<DistributedTrainer> trainer, int rank);
    
    // Run distributed training
    void train(const std::vector<std::vector<double>>& data,
              const std::vector<std::vector<double>>& labels,
              int epochs,
              int batch_size = 32);
    
    // Get training statistics
    std::vector<double> get_loss_history() const { return loss_history_; }
    std::vector<double> get_accuracy_history() const { return accuracy_history_; }
    
    // Model checkpointing
    void save_checkpoint(const std::string& path);
    void load_checkpoint(const std::string& path);
    
private:
    int world_size_;
    std::map<int, std::shared_ptr<DistributedTrainer>> trainers_;
    std::vector<double> loss_history_;
    std::vector<double> accuracy_history_;
    
    std::shared_ptr<ParameterServer> param_server_;
};

// Utility functions
namespace utils {
    // Split data across workers
    std::vector<std::vector<std::vector<double>>> 
    partition_data(const std::vector<std::vector<double>>& data, int world_size);
    
    // Compute metrics across distributed workers
    double compute_distributed_accuracy(
        const std::vector<std::vector<double>>& predictions,
        const std::vector<std::vector<double>>& labels,
        std::shared_ptr<DistributedContext> context);
    
    // Load balancing
    std::vector<int> balance_workload(const std::vector<int>& task_costs, int world_size);
}

} // namespace distributed
} // namespace networking
