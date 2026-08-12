#include "tyst_framework.hpp"

#include "distributed_owa.h"

#include <cmath>
#include <limits>

using namespace opt;
using ml::deep_learning::Tensor;

namespace {

double parameter_loss(const std::vector<double>& parameters, const std::vector<double>& target) {
    double loss = 0.0;
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        const double delta = parameters[i] - target[i];
        loss += delta * delta;
    }
    return loss;
}

class CandidateSearchOptimizer : public OptimizationAlgorithm {
public:
    std::vector<double> optimize(const ObjectiveFunction& objective, const std::vector<double>& initial_state) override {
        best_solution_ = initial_state;
        best_score_ = objective(initial_state);
        for (std::size_t i = 0; i < initial_state.size(); ++i) {
            std::vector<double> candidate(initial_state.size(), 0.0);
            candidate[i] = 1.0;
            const double score = objective(candidate);
            if (score < best_score_) {
                best_score_ = score;
                best_solution_ = candidate;
            }
        }
        return best_solution_;
    }

    const std::vector<double>& best_solution() const override { return best_solution_; }
    double best_score() const override { return best_score_; }

private:
    std::vector<double> best_solution_;
    double best_score_ = std::numeric_limits<double>::infinity();
};

} // namespace

TYST_TEST(DistributedOwaTest, LinearOwaLearnsBetterWeightsThanNaiveAverage) {
    const std::vector<std::vector<double>> local_models = {
        {0.0, 0.0},
        {2.0, 1.0},
        {4.0, 3.0}
    };
    const std::vector<std::vector<double>> features = {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};
    const std::vector<double> targets = {2.0, 1.0, 3.0};

    OwaConfig config;
    config.merge_iterations = 800;
    config.learning_rate = 0.05;
    const OwaResult owa = optimal_weighted_average_linear_regression(local_models, features, targets, config);
    const std::vector<double> naive = naive_average_parameters(local_models);

    TYST_EXPECT_EQ(owa.model_weights.size(), local_models.size());
    TYST_EXPECT_TRUE(parameter_loss(owa.merged_parameters, {2.0, 1.0}) < parameter_loss(naive, {2.0, 1.0}));
}

TYST_TEST(DistributedOwaTest, FastCrossValidationProducesOneFoldPerMachine) {
    const std::vector<std::vector<double>> local_models = {{0.0, 0.0}, {2.0, 1.0}, {4.0, 3.0}};
    const std::vector<std::vector<std::vector<double>>> fold_features = {
        {{1.0, 0.0}},
        {{0.0, 1.0}},
        {{1.0, 1.0}}
    };
    const std::vector<std::vector<double>> fold_targets = {{2.0}, {1.0}, {3.0}};

    const OwaCrossValidationResult result = fast_owa_cross_validation_linear_regression(
        local_models,
        fold_features,
        fold_targets);

    TYST_EXPECT_EQ(result.fold_losses.size(), static_cast<std::size_t>(3));
    TYST_EXPECT_TRUE(std::isfinite(result.mean_loss));
}

TYST_TEST(DistributedOwaTest, NowaMergesDeepLearningLayerTensorsIndependently) {
    const std::vector<std::vector<Tensor>> local_models = {
        {Tensor({2}, {0.0, 0.0}), Tensor({1}, {0.0})},
        {Tensor({2}, {2.0, 1.0}), Tensor({1}, {3.0})},
        {Tensor({2}, {4.0, 3.0}), Tensor({1}, {6.0})}
    };

    auto validation_loss = [](const std::vector<Tensor>& layers) {
        double loss = 0.0;
        const std::vector<double> first_target = {2.0, 1.0};
        for (std::size_t i = 0; i < layers[0].size(); ++i) {
            const double delta = layers[0].data()[i] - first_target[i];
            loss += delta * delta;
        }
        const double second_delta = layers[1].data()[0] - 3.0;
        return loss + second_delta * second_delta;
    };

    OwaConfig config;
    config.merge_iterations = 80;
    const NowaResult nowa = nonlinear_optimal_weighted_average_layers(local_models, validation_loss, config);
    const std::vector<Tensor> naive_layers = {Tensor({2}, {2.0, 4.0 / 3.0}), Tensor({1}, {3.0})};

    TYST_EXPECT_EQ(nowa.merged_layers.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(nowa.layer_weights.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(nowa.merged_layers[0].shape(), std::vector<std::size_t>({2}));
    TYST_EXPECT_EQ(nowa.merged_layers[1].shape(), std::vector<std::size_t>({1}));
    TYST_EXPECT_TRUE(nowa.validation_loss < validation_loss(naive_layers));
}

TYST_TEST(DistributedOwaTest, NowaAcceptsOptimizationAlgorithmImplementations) {
    const std::vector<std::vector<Tensor>> local_models = {
        {Tensor({2}, {0.0, 0.0}), Tensor({1}, {0.0})},
        {Tensor({2}, {2.0, 1.0}), Tensor({1}, {3.0})},
        {Tensor({2}, {5.0, 5.0}), Tensor({1}, {8.0})}
    };

    auto validation_loss = [](const std::vector<Tensor>& layers) {
        const double first_delta = layers[0].data()[0] - 2.0;
        const double second_delta = layers[0].data()[1] - 1.0;
        const double output_delta = layers[1].data()[0] - 3.0;
        return first_delta * first_delta + second_delta * second_delta + output_delta * output_delta;
    };

    CandidateSearchOptimizer optimizer;
    const NowaResult nowa = nonlinear_optimal_weighted_average_layers(local_models, validation_loss, optimizer);

    TYST_EXPECT_NEAR(nowa.validation_loss, 0.0, 1e-9);
    TYST_EXPECT_NEAR(nowa.layer_weights[0][1], 1.0, 1e-9);
    TYST_EXPECT_NEAR(nowa.layer_weights[1][1], 1.0, 1e-9);
}