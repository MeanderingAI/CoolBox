#include "simulated_annealing.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace {

double clamp_value(double value, double lo, double hi) {
    return std::max(lo, std::min(hi, value));
}

} // namespace

namespace opt {

SimulatedAnnealing::SimulatedAnnealing(const Config& config)
    : config_(config),
      best_score_(std::numeric_limits<double>::infinity()) {
    if (config_.dimensions <= 0 || config_.iterations_per_temperature <= 0) {
        throw std::invalid_argument("SimulatedAnnealing: invalid optimization configuration");
    }
    if (config_.initial_temperature <= config_.minimum_temperature) {
        throw std::invalid_argument("SimulatedAnnealing: initial_temperature must be larger than minimum_temperature");
    }
    if (config_.cooling_rate <= 0.0 || config_.cooling_rate >= 1.0) {
        throw std::invalid_argument("SimulatedAnnealing: cooling_rate must be in (0, 1)");
    }
}

std::vector<double> SimulatedAnnealing::optimize(
    const ObjectiveFunction& objective,
    const std::vector<double>& initial_state)
{
    if (static_cast<int>(initial_state.size()) != config_.dimensions) {
        throw std::invalid_argument("SimulatedAnnealing: initial_state size mismatch");
    }

    std::mt19937 rng(config_.seed);
    std::uniform_real_distribution<double> unit01(0.0, 1.0);
    std::normal_distribution<double> gaussian(0.0, config_.step_sigma);

    std::vector<double> current = initial_state;
    for (double& x : current) {
        x = clamp_value(x, config_.lower_bound, config_.upper_bound);
    }

    double current_score = objective(current);
    best_solution_ = current;
    best_score_ = current_score;

    double temperature = config_.initial_temperature;
    while (temperature > config_.minimum_temperature) {
        for (int i = 0; i < config_.iterations_per_temperature; ++i) {
            std::vector<double> candidate = current;
            for (int d = 0; d < config_.dimensions; ++d) {
                candidate[static_cast<std::size_t>(d)] += gaussian(rng);
                candidate[static_cast<std::size_t>(d)] = clamp_value(
                    candidate[static_cast<std::size_t>(d)],
                    config_.lower_bound,
                    config_.upper_bound);
            }

            const double candidate_score = objective(candidate);
            const double delta = candidate_score - current_score;

            bool accept = false;
            if (delta <= 0.0) {
                accept = true;
            } else {
                const double probability = std::exp(-delta / temperature);
                accept = unit01(rng) < probability;
            }

            if (accept) {
                current = candidate;
                current_score = candidate_score;
                if (current_score < best_score_) {
                    best_score_ = current_score;
                    best_solution_ = current;
                }
            }
        }

        temperature *= config_.cooling_rate;
    }

    return best_solution_;
}

const std::vector<double>& SimulatedAnnealing::best_solution() const {
    return best_solution_;
}

double SimulatedAnnealing::best_score() const {
    return best_score_;
}

} // namespace opt
