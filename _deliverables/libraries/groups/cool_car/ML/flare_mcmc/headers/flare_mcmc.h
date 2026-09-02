#ifndef FLARE_MCMC_H
#define FLARE_MCMC_H

#include <cstddef>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace ml {

using FlareState = std::vector<double>;
using FlareLogDensity = std::function<double(const FlareState&)>;

struct FlareLayer {
    std::string name;
    FlareLogDensity log_density;
    double tuning_omega = 0.0;
    double evaluation_cost = 1.0;
};

struct FlareConfig {
    std::size_t inner_steps = 4;
    double proposal_stddev = 1.0;
    bool enable_layer_tuning = false;
    double tuning_learning_rate = 1e-3;
    double min_tuning_omega = 0.0;
    unsigned int seed = 5489u;
};

struct FlareLayerStats {
    std::size_t proposals = 0;
    std::size_t accepted = 0;
    std::size_t log_density_evaluations = 0;

    double acceptance_rate() const;
};

struct FlareRunResult {
    std::vector<FlareState> samples;
    std::vector<FlareLayerStats> layer_stats;
    std::vector<double> tuning_omegas;
};

class FlareMcmcSampler {
public:
    FlareMcmcSampler(std::vector<FlareLayer> layers, FlareConfig config = FlareConfig());

    FlareRunResult sample(const FlareState& initial_state, std::size_t sample_count);

    double effective_log_density(std::size_t layer_index, const FlareState& state) const;
    double acceptance_log_ratio(std::size_t layer_index,
                                const FlareState& current,
                                const FlareState& proposal) const;

    const std::vector<FlareLayer>& layers() const { return layers_; }
    const FlareConfig& config() const { return config_; }

private:
    struct ChainStep {
        FlareState state;
        double log_acceptance_ratio = 0.0;
        bool accepted = false;
    };

    std::vector<FlareLayer> layers_;
    FlareConfig config_;
    mutable std::vector<FlareLayerStats> stats_;
    std::mt19937 rng_;

    std::vector<FlareState> run_chain(std::size_t layer_index,
                                      const FlareState& initial_state,
                                      std::size_t sample_count);
    ChainStep step_chain(std::size_t layer_index, const FlareState& current);
    FlareState gaussian_proposal(const FlareState& current);
    bool accept(double log_acceptance_ratio);
    void update_tuning(std::size_t approximating_layer,
                       const FlareState& start_state,
                       const FlareState& end_state);
    double raw_log_density(std::size_t layer_index, const FlareState& state) const;
    double raw_density(std::size_t layer_index, const FlareState& state) const;
    void validate_state(const FlareState& state) const;
};

} // namespace ml

#endif // FLARE_MCMC_H