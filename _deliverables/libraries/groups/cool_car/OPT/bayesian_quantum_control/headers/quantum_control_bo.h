#ifndef QUANTUM_CONTROL_BO_H
#define QUANTUM_CONTROL_BO_H

#include "optimization_algorithm.h"

#include <random>
#include <vector>

namespace opt {

enum class QuantumControlSearchPolicy {
    Standard,
    Crop,
    CropAndExpand,
    Warp
};

class QuantumControlBayesianOptimizer : public OptimizationAlgorithm {
public:
    struct SearchBounds {
        double frequency_center = 1.0;
        double initial_frequency_ratio = 0.5;
        double current_frequency_ratio = 0.5;
        double amplitude_min = 0.0;
        double amplitude_max = 0.15;
    };

    struct Config {
        QuantumControlSearchPolicy policy = QuantumControlSearchPolicy::Warp;
        SearchBounds bounds;
        int initial_samples = 8;
        int iterations = 40;
        int acquisition_candidates = 128;
        double exploration_beta = 1.5;
        double length_scale_frequency = 0.15;
        double length_scale_amplitude = 0.05;
        double observation_noise = 1e-6;
        double crop_min_ratio = 0.01;
        double expansion_rate = 0.2;
        unsigned int seed = 431;
    };

    QuantumControlBayesianOptimizer();
    explicit QuantumControlBayesianOptimizer(const Config& config);

    std::vector<double> optimize(
        const ObjectiveFunction& objective,
        const std::vector<double>& initial_state) override;

    const std::vector<double>& best_solution() const override;
    double best_score() const override;

    SearchBounds current_bounds() const;
    double best_probability() const;
    std::vector<double> warp_frequency_point(const std::vector<double>& point) const;
    std::vector<double> unwarp_frequency_point(const std::vector<double>& point) const;

private:
    struct Observation {
        std::vector<double> point;
        double probability;
    };

    Config config_;
    SearchBounds current_bounds_;
    mutable std::mt19937 rng_;
    std::vector<double> best_solution_;
    double best_score_;
    double best_probability_;

    std::vector<double> sample_point() const;
    std::vector<double> propose_candidate(const std::vector<Observation>& observations) const;
    void update_search_policy(const std::vector<double>& point, double probability);
    double gaussian_process_ucb(
        const std::vector<Observation>& observations,
        const std::vector<double>& candidate) const;
};

const char* to_string(QuantumControlSearchPolicy policy);

} // namespace opt

#endif // QUANTUM_CONTROL_BO_H