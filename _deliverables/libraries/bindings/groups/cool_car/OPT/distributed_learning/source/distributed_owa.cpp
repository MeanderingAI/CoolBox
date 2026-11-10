#include "distributed_owa.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {

void validate_local_parameters(const std::vector<std::vector<double>>& local_parameters) {
    if (local_parameters.empty() || local_parameters.front().empty()) {
        throw std::invalid_argument("OWA requires at least one non-empty local parameter vector");
    }
    const std::size_t dimension = local_parameters.front().size();
    for (const auto& parameters : local_parameters) {
        if (parameters.size() != dimension) {
            throw std::invalid_argument("OWA local parameter vectors must have matching dimensions");
        }
    }
}

std::vector<double> project_simplex(std::vector<double> values) {
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());
    double prefix = 0.0;
    double theta = 0.0;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        prefix += sorted[i];
        const double candidate = (prefix - 1.0) / static_cast<double>(i + 1);
        if (sorted[i] > candidate) theta = candidate;
    }
    for (double& value : values) value = std::max(0.0, value - theta);
    return values;
}

std::vector<double> combine_parameters(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<double>& weights) {
    std::vector<double> merged(local_parameters.front().size(), 0.0);
    for (std::size_t model = 0; model < local_parameters.size(); ++model) {
        for (std::size_t d = 0; d < merged.size(); ++d) {
            merged[d] += weights[model] * local_parameters[model][d];
        }
    }
    return merged;
}

double dot_product(const std::vector<double>& left, const std::vector<double>& right) {
    return std::inner_product(left.begin(), left.end(), right.begin(), 0.0);
}

double squared_loss(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& targets,
    const std::vector<double>& weights,
    double l2_regularization) {
    const std::vector<double> merged = combine_parameters(local_parameters, weights);
    double loss = 0.0;
    for (std::size_t row = 0; row < features.size(); ++row) {
        const double residual = dot_product(features[row], merged) - targets[row];
        loss += residual * residual;
    }
    loss /= std::max<std::size_t>(1, features.size());
    loss += l2_regularization * dot_product(weights, weights);
    return loss;
}

void validate_tensor_stack(const std::vector<ml::deep_learning::Tensor>& tensors) {
    if (tensors.empty()) throw std::invalid_argument("NOWA requires at least one tensor per layer");
    const auto shape = tensors.front().shape();
    for (const auto& tensor : tensors) {
        if (tensor.shape() != shape) {
            throw std::invalid_argument("NOWA tensors for each layer must have matching shapes");
        }
    }
}

void validate_owa_regression_inputs(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& validation_features,
    const std::vector<double>& validation_targets) {
    validate_local_parameters(local_parameters);
    if (validation_features.size() != validation_targets.size() || validation_features.empty()) {
        throw std::invalid_argument("OWA validation features and targets must be non-empty and aligned");
    }
    for (const auto& row : validation_features) {
        if (row.size() != local_parameters.front().size()) {
            throw std::invalid_argument("OWA validation feature dimension must match local parameters");
        }
    }
}

std::vector<double> normalized_weights(std::vector<double> weights, bool project_to_simplex) {
    if (project_to_simplex) return project_simplex(std::move(weights));
    return weights;
}

std::vector<double> optimize_weights_by_coordinate_search(
    std::size_t model_count,
    const std::function<double(const std::vector<double>&)>& objective,
    const opt::OwaConfig& config) {
    std::vector<double> weights(model_count, 1.0 / static_cast<double>(model_count));
    double best_loss = objective(weights);
    double step = 0.25;
    for (int iter = 0; iter < config.merge_iterations; ++iter) {
        bool improved = false;
        for (std::size_t i = 0; i < model_count; ++i) {
            for (std::size_t j = 0; j < model_count; ++j) {
                if (i == j) continue;
                std::vector<double> candidate = weights;
                const double delta = std::min(step, candidate[j]);
                candidate[i] += delta;
                candidate[j] -= delta;
                if (config.project_to_simplex) candidate = project_simplex(candidate);
                const double candidate_loss = objective(candidate);
                if (candidate_loss + 1e-12 < best_loss) {
                    best_loss = candidate_loss;
                    weights = candidate;
                    improved = true;
                }
            }
        }
        if (!improved) step *= 0.5;
        if (step < 1e-6) break;
    }
    return weights;
}

} // namespace

namespace opt {

std::vector<double> naive_average_parameters(const std::vector<std::vector<double>>& local_parameters) {
    validate_local_parameters(local_parameters);
    std::vector<double> merged(local_parameters.front().size(), 0.0);
    for (const auto& parameters : local_parameters) {
        for (std::size_t d = 0; d < merged.size(); ++d) merged[d] += parameters[d];
    }
    for (double& value : merged) value /= static_cast<double>(local_parameters.size());
    return merged;
}

OwaResult optimal_weighted_average_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& validation_features,
    const std::vector<double>& validation_targets,
    const OwaConfig& config) {
    validate_owa_regression_inputs(local_parameters, validation_features, validation_targets);

    std::vector<double> weights(local_parameters.size(), 1.0 / static_cast<double>(local_parameters.size()));
    for (int iter = 0; iter < config.merge_iterations; ++iter) {
        std::vector<double> gradient(weights.size(), 0.0);
        for (std::size_t row = 0; row < validation_features.size(); ++row) {
            const std::vector<double> merged = combine_parameters(local_parameters, weights);
            const double residual = dot_product(validation_features[row], merged) - validation_targets[row];
            for (std::size_t model = 0; model < local_parameters.size(); ++model) {
                gradient[model] += 2.0 * residual * dot_product(validation_features[row], local_parameters[model]);
            }
        }
        for (std::size_t model = 0; model < weights.size(); ++model) {
            gradient[model] /= static_cast<double>(validation_features.size());
            gradient[model] += 2.0 * config.l2_regularization * weights[model];
            weights[model] -= config.learning_rate * gradient[model];
        }
        if (config.project_to_simplex) weights = project_simplex(weights);
    }

    OwaResult result;
    result.model_weights = weights;
    result.merged_parameters = combine_parameters(local_parameters, weights);
    result.validation_loss = squared_loss(local_parameters, validation_features, validation_targets, weights, config.l2_regularization);
    return result;
}

OwaResult optimal_weighted_average_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<double>>& validation_features,
    const std::vector<double>& validation_targets,
    OptimizationAlgorithm& optimizer,
    const OwaConfig& config) {
    validate_owa_regression_inputs(local_parameters, validation_features, validation_targets);

    std::vector<double> initial_weights(local_parameters.size(), 1.0 / static_cast<double>(local_parameters.size()));
    auto objective = [&](const std::vector<double>& raw_weights) {
        const std::vector<double> weights = normalized_weights(raw_weights, config.project_to_simplex);
        return squared_loss(local_parameters, validation_features, validation_targets, weights, config.l2_regularization);
    };
    std::vector<double> weights = normalized_weights(optimizer.optimize(objective, initial_weights), config.project_to_simplex);

    OwaResult result;
    result.model_weights = weights;
    result.merged_parameters = combine_parameters(local_parameters, weights);
    result.validation_loss = squared_loss(local_parameters, validation_features, validation_targets, weights, config.l2_regularization);
    return result;
}

OwaCrossValidationResult fast_owa_cross_validation_linear_regression(
    const std::vector<std::vector<double>>& local_parameters,
    const std::vector<std::vector<std::vector<double>>>& fold_features,
    const std::vector<std::vector<double>>& fold_targets,
    const OwaConfig& config) {
    validate_local_parameters(local_parameters);
    if (fold_features.size() != local_parameters.size() || fold_targets.size() != local_parameters.size()) {
        throw std::invalid_argument("OWA fast CV expects one fold per local model");
    }

    OwaCrossValidationResult result;
    for (std::size_t held_out = 0; held_out < local_parameters.size(); ++held_out) {
        std::vector<std::vector<double>> train_parameters;
        for (std::size_t model = 0; model < local_parameters.size(); ++model) {
            if (model != held_out) train_parameters.push_back(local_parameters[model]);
        }
        const OwaResult fold = optimal_weighted_average_linear_regression(
            train_parameters,
            fold_features[held_out],
            fold_targets[held_out],
            config);
        double loss = 0.0;
        for (std::size_t row = 0; row < fold_features[held_out].size(); ++row) {
            const double residual = dot_product(fold_features[held_out][row], fold.merged_parameters) - fold_targets[held_out][row];
            loss += residual * residual;
        }
        loss /= std::max<std::size_t>(1, fold_features[held_out].size());
        result.fold_losses.push_back(loss);
    }
    result.mean_loss = std::accumulate(result.fold_losses.begin(), result.fold_losses.end(), 0.0)
        / static_cast<double>(result.fold_losses.size());
    return result;
}

ml::deep_learning::Tensor weighted_tensor_average(
    const std::vector<ml::deep_learning::Tensor>& local_tensors,
    const std::vector<double>& weights) {
    validate_tensor_stack(local_tensors);
    if (local_tensors.size() != weights.size()) {
        throw std::invalid_argument("NOWA tensor count must match weight count");
    }
    ml::deep_learning::Tensor merged(local_tensors.front().shape(), 0.0);
    for (std::size_t model = 0; model < local_tensors.size(); ++model) {
        for (std::size_t i = 0; i < merged.size(); ++i) {
            merged.data()[i] += weights[model] * local_tensors[model].data()[i];
        }
    }
    return merged;
}

NowaResult nonlinear_optimal_weighted_average_layers(
    const std::vector<std::vector<ml::deep_learning::Tensor>>& local_model_layers,
    const NowaLayerLoss& validation_loss,
    const OwaConfig& config) {
    if (local_model_layers.empty() || local_model_layers.front().empty()) {
        throw std::invalid_argument("NOWA requires at least one local model with layers");
    }
    const std::size_t model_count = local_model_layers.size();
    const std::size_t layer_count = local_model_layers.front().size();
    for (const auto& model : local_model_layers) {
        if (model.size() != layer_count) throw std::invalid_argument("NOWA local models must have matching layer counts");
    }

    NowaResult result;
    result.merged_layers.resize(layer_count);
    result.layer_weights.resize(layer_count);
    std::vector<double> uniform(model_count, 1.0 / static_cast<double>(model_count));
    for (std::size_t layer = 0; layer < layer_count; ++layer) {
        std::vector<ml::deep_learning::Tensor> layer_tensors;
        for (const auto& model : local_model_layers) layer_tensors.push_back(model[layer]);
        validate_tensor_stack(layer_tensors);
        result.merged_layers[layer] = weighted_tensor_average(layer_tensors, uniform);
    }

    for (std::size_t layer = 0; layer < layer_count; ++layer) {
        std::vector<ml::deep_learning::Tensor> layer_tensors;
        for (const auto& model : local_model_layers) layer_tensors.push_back(model[layer]);
        auto objective = [&](const std::vector<double>& weights) {
            std::vector<ml::deep_learning::Tensor> candidate_layers = result.merged_layers;
            candidate_layers[layer] = weighted_tensor_average(layer_tensors, weights);
            return validation_loss(candidate_layers) + config.l2_regularization * dot_product(weights, weights);
        };
        const std::vector<double> weights = optimize_weights_by_coordinate_search(model_count, objective, config);
        result.layer_weights[layer] = weights;
        result.merged_layers[layer] = weighted_tensor_average(layer_tensors, weights);
    }
    result.validation_loss = validation_loss(result.merged_layers);
    return result;
}

NowaResult nonlinear_optimal_weighted_average_layers(
    const std::vector<std::vector<ml::deep_learning::Tensor>>& local_model_layers,
    const NowaLayerLoss& validation_loss,
    OptimizationAlgorithm& optimizer,
    const OwaConfig& config) {
    if (local_model_layers.empty() || local_model_layers.front().empty()) {
        throw std::invalid_argument("NOWA requires at least one local model with layers");
    }
    const std::size_t model_count = local_model_layers.size();
    const std::size_t layer_count = local_model_layers.front().size();
    for (const auto& model : local_model_layers) {
        if (model.size() != layer_count) throw std::invalid_argument("NOWA local models must have matching layer counts");
    }

    NowaResult result;
    result.merged_layers.resize(layer_count);
    result.layer_weights.resize(layer_count);
    std::vector<double> uniform(model_count, 1.0 / static_cast<double>(model_count));
    for (std::size_t layer = 0; layer < layer_count; ++layer) {
        std::vector<ml::deep_learning::Tensor> layer_tensors;
        for (const auto& model : local_model_layers) layer_tensors.push_back(model[layer]);
        validate_tensor_stack(layer_tensors);
        result.merged_layers[layer] = weighted_tensor_average(layer_tensors, uniform);
    }

    for (std::size_t layer = 0; layer < layer_count; ++layer) {
        std::vector<ml::deep_learning::Tensor> layer_tensors;
        for (const auto& model : local_model_layers) layer_tensors.push_back(model[layer]);
        auto objective = [&](const std::vector<double>& raw_weights) {
            const std::vector<double> weights = normalized_weights(raw_weights, config.project_to_simplex);
            std::vector<ml::deep_learning::Tensor> candidate_layers = result.merged_layers;
            candidate_layers[layer] = weighted_tensor_average(layer_tensors, weights);
            return validation_loss(candidate_layers) + config.l2_regularization * dot_product(weights, weights);
        };
        const std::vector<double> weights = normalized_weights(optimizer.optimize(objective, uniform), config.project_to_simplex);
        result.layer_weights[layer] = weights;
        result.merged_layers[layer] = weighted_tensor_average(layer_tensors, weights);
    }
    result.validation_loss = validation_loss(result.merged_layers);
    return result;
}

} // namespace opt