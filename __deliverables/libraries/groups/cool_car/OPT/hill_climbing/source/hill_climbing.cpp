#include "hill_climbing.h"

#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>

namespace {

double clamp_value(double value, double lo, double hi) {
    return std::max(lo, std::min(hi, value));
}

} // namespace

namespace opt {

HillClimbing::HillClimbing(const Config& config)
    : config_(config),
      best_score_(std::numeric_limits<double>::infinity()) {
    if (config_.dimensions <= 0 || config_.max_iterations <= 0 || config_.neighbours_per_iteration <= 0) {
        throw std::invalid_argument("HillClimbing: invalid optimization configuration");
    }
    if (config_.lower_bound >= config_.upper_bound) {
        throw std::invalid_argument("HillClimbing: lower_bound must be less than upper_bound");
    }
}

std::vector<double> HillClimbing::optimize(
    const ObjectiveFunction& objective,
    const std::vector<double>& initial_state)
{
    if (static_cast<int>(initial_state.size()) != config_.dimensions) {
        throw std::invalid_argument("HillClimbing: initial_state size mismatch");
    }

    std::mt19937 rng(config_.seed);
    std::uniform_real_distribution<double> uniform(config_.lower_bound, config_.upper_bound);
    std::normal_distribution<double> gaussian(0.0, config_.step_sigma);

    auto run_from = [&](std::vector<double> current) {
        for (double& x : current) {
            x = clamp_value(x, config_.lower_bound, config_.upper_bound);
        }

        double current_score = objective(current);

        for (int iter = 0; iter < config_.max_iterations; ++iter) {
            std::vector<double> best_neighbor = current;
            double best_neighbor_score = current_score;

            for (int n = 0; n < config_.neighbours_per_iteration; ++n) {
                std::vector<double> candidate = current;
                for (int d = 0; d < config_.dimensions; ++d) {
                    candidate[static_cast<std::size_t>(d)] += gaussian(rng);
                    candidate[static_cast<std::size_t>(d)] = clamp_value(
                        candidate[static_cast<std::size_t>(d)],
                        config_.lower_bound,
                        config_.upper_bound);
                }

                const double score = objective(candidate);
                if (score < best_neighbor_score) {
                    best_neighbor = candidate;
                    best_neighbor_score = score;
                }
            }

            if (best_neighbor_score >= current_score) {
                break;
            }

            current = best_neighbor;
            current_score = best_neighbor_score;
        }

        if (current_score < best_score_) {
            best_score_ = current_score;
            best_solution_ = current;
        }
    };

    best_solution_.clear();
    best_score_ = std::numeric_limits<double>::infinity();

    run_from(initial_state);
    for (int restart = 0; restart < config_.max_restarts; ++restart) {
        std::vector<double> random_start(static_cast<std::size_t>(config_.dimensions), 0.0);
        for (double& x : random_start) {
            x = uniform(rng);
        }
        run_from(random_start);
    }

    return best_solution_;
}

const std::vector<double>& HillClimbing::best_solution() const {
    return best_solution_;
}

double HillClimbing::best_score() const {
    return best_score_;
}

} // namespace opt
