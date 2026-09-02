#ifndef DISTRIBUTED_OWA_H
#define DISTRIBUTED_OWA_H

#include "optimization_algorithm.h"
#include "tensor.h"

#include <functional>
#include <vector>

namespace opt {

struct OwaConfig {
    int merge_iterations = 500;
    double learning_rate = 0.05;
    double l2_regularization = 1e-4;
    bool project_to_simplex = true;
};

struct OwaResult {
    std::vector<double> merged_parameters;
    std::vector<double> model_weights;
    double validation_loss = 0.0;
};

struct OwaCrossValidationResult {
    double mean_loss = 0.0;
    std::vector<double> fold_losses;
};

using NowaLayerLoss = std::function<double(const std::vector<ml::deep_learning::Tensor>&)>;

struct NowaResult {
    std::vector<ml::deep_learning::Tensor> merged_layers;
    std::vector<std::vector<double>> layer_weights;
    double validation_loss = 0.0;
};

std::vector<double> naive_average_parameters(const std::vector<std::vector<double>>& local_parameters);

OwaResult optimal_weighted_average_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& validation_features,
    const std::vector<double>& validation_targets,
    const OwaConfig& config = OwaConfig());

OwaResult optimal_weighted_average_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& validation_features,
    const std::vector<double>& validation_targets,
    OptimizationAlgorithm& optimizer,
    const OwaConfig& config = OwaConfig());

OwaCrossValidationResult fast_owa_cross_validation_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<std::vector<double>>>& fold_features,
    const std::vector<std::vector<double>>& fold_targets,
    const OwaConfig& config = OwaConfig());

ml::deep_learning::Tensor weighted_tensor_average(
    const std::vector<ml::deep_learning::Tensor>& local_tensors,
    const std::vector<double>& weights);

NowaResult nonlinear_optimal_weighted_average_layers(
    const std::vector<std::vector<ml::deep_learning::Tensor>>& local_model_layers,
    const NowaLayerLoss& validation_loss,
    const OwaConfig& config = OwaConfig());

NowaResult nonlinear_optimal_weighted_average_layers(
    const std::vector<std::vector<ml::deep_learning::Tensor>>& local_model_layers,
    const NowaLayerLoss& validation_loss,
    OptimizationAlgorithm& optimizer,
    const OwaConfig& config = OwaConfig());

} // namespace opt

#endif // DISTRIBUTED_OWA_H