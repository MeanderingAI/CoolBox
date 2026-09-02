#include "tyst_framework.hpp"
#include "flare_mcmc.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace {

double normal_log_density(double mean, double variance, const ml::FlareState& state) {
    const double diff = state[0] - mean;
    return -0.5 * diff * diff / variance;
}

} // namespace

TEST(FlareMcmc, ComputesRecursiveAcceptanceCorrection) {
    std::vector<ml::FlareLayer> layers = {
        {"fine", [](const ml::FlareState& state) { return normal_log_density(2.0, 1.0, state); }},
        {"coarse", [](const ml::FlareState& state) { return normal_log_density(1.0, 1.0, state); }}
    };

    ml::FlareMcmcSampler sampler(layers);
    const double log_ratio = sampler.acceptance_log_ratio(0, {0.0}, {1.0});
    EXPECT_NEAR(log_ratio, 1.0, 1e-12);
}

TEST(FlareMcmc, SamplesFromFinestLayerWithNestedCoarseProposal) {
    std::vector<ml::FlareLayer> layers = {
        {"fine", [](const ml::FlareState& state) { return normal_log_density(1.5, 1.0, state); }},
        {"middle", [](const ml::FlareState& state) { return normal_log_density(1.0, 1.5, state); }},
        {"coarse", [](const ml::FlareState& state) { return normal_log_density(0.5, 2.0, state); }}
    };

    ml::FlareConfig config;
    config.seed = 7u;
    config.inner_steps = 3;
    config.proposal_stddev = 0.7;

    ml::FlareMcmcSampler sampler(layers, config);
    ml::FlareRunResult result = sampler.sample({0.0}, 80);

    EXPECT_EQ(result.samples.size(), 80u);
    EXPECT_EQ(result.layer_stats.size(), 3u);
    EXPECT_EQ(result.layer_stats[0].proposals, 80u);
    EXPECT_GT(result.layer_stats[1].proposals, result.layer_stats[0].proposals);
    EXPECT_GT(result.layer_stats[2].proposals, result.layer_stats[1].proposals);
    EXPECT_GT(result.layer_stats[0].accepted, 0u);

    double mean = 0.0;
    for (const auto& sample : result.samples) {
        EXPECT_EQ(sample.size(), 1u);
        mean += sample[0];
    }
    mean /= static_cast<double>(result.samples.size());
    EXPECT_GT(mean, -0.5);
    EXPECT_LT(mean, 2.8);
}

TEST(FlareMcmc, LayerTuningChangesCoarseOmega) {
    std::vector<ml::FlareLayer> layers = {
        {"fine", [](const ml::FlareState& state) { return normal_log_density(1.5, 1.0, state); }},
        {"coarse", [](const ml::FlareState& state) { return normal_log_density(-1.0, 2.0, state); }, 0.25}
    };

    ml::FlareConfig config;
    config.seed = 11u;
    config.inner_steps = 4;
    config.proposal_stddev = 0.8;
    config.enable_layer_tuning = true;
    config.tuning_learning_rate = 1e-3;
    config.min_tuning_omega = 0.0;

    ml::FlareMcmcSampler sampler(layers, config);
    ml::FlareRunResult result = sampler.sample({0.0}, 40);

    EXPECT_EQ(result.tuning_omegas.size(), 2u);
    EXPECT_DOUBLE_EQ(result.tuning_omegas[0], 0.0);
    EXPECT_NE(result.tuning_omegas[1], 0.25);
    EXPECT_GE(result.tuning_omegas[1], 0.0);
}