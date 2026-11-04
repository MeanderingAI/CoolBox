#include "genetic_search.h"

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

GeneticSearch::GeneticSearch(const Config& config)
    : config_(config),
      best_score_(std::numeric_limits<double>::infinity()) {
    if (config_.dimensions <= 0 || config_.population_size < 2 || config_.generations <= 0) {
        throw std::invalid_argument("GeneticSearch: invalid optimization configuration");
    }
    if (config_.lower_bound >= config_.upper_bound) {
        throw std::invalid_argument("GeneticSearch: lower_bound must be less than upper_bound");
    }
}

std::vector<double> GeneticSearch::optimize(const ObjectiveFunction& objective) {
    return optimize(objective, {});
}

std::vector<double> GeneticSearch::optimize(
    const ObjectiveFunction& objective,
    const std::vector<double>& initial_state)
{
    std::mt19937 rng(config_.seed);
    std::uniform_real_distribution<double> uniform(config_.lower_bound, config_.upper_bound);
    std::uniform_real_distribution<double> unit01(0.0, 1.0);
    std::normal_distribution<double> gaussian(0.0, config_.mutation_sigma);

    std::vector<std::vector<double>> population(
        static_cast<std::size_t>(config_.population_size),
        std::vector<double>(static_cast<std::size_t>(config_.dimensions), 0.0));
    std::vector<double> scores(static_cast<std::size_t>(config_.population_size), 0.0);

    for (auto& individual : population) {
        for (double& x : individual) {
            x = uniform(rng);
        }
    }

    if (!initial_state.empty()) {
        if (static_cast<int>(initial_state.size()) != config_.dimensions) {
            throw std::invalid_argument("GeneticSearch: initial_state size mismatch");
        }
        population[0] = initial_state;
        for (double& x : population[0]) {
            x = clamp_value(x, config_.lower_bound, config_.upper_bound);
        }
    }

    auto evaluate_population = [&]() {
        for (int i = 0; i < config_.population_size; ++i) {
            scores[static_cast<std::size_t>(i)] = objective(population[static_cast<std::size_t>(i)]);
            if (scores[static_cast<std::size_t>(i)] < best_score_) {
                best_score_ = scores[static_cast<std::size_t>(i)];
                best_solution_ = population[static_cast<std::size_t>(i)];
            }
        }
    };

    auto tournament_pick = [&]() {
        std::uniform_int_distribution<int> pick(0, config_.population_size - 1);
        int best_idx = pick(rng);
        for (int k = 1; k < config_.tournament_size; ++k) {
            const int idx = pick(rng);
            if (scores[static_cast<std::size_t>(idx)] < scores[static_cast<std::size_t>(best_idx)]) {
                best_idx = idx;
            }
        }
        return population[static_cast<std::size_t>(best_idx)];
    };

    evaluate_population();

    for (int gen = 0; gen < config_.generations; ++gen) {
        std::vector<std::vector<double>> next_population;
        next_population.reserve(static_cast<std::size_t>(config_.population_size));

        next_population.push_back(best_solution_);

        while (static_cast<int>(next_population.size()) < config_.population_size) {
            std::vector<double> p1 = tournament_pick();
            std::vector<double> p2 = tournament_pick();

            std::vector<double> child = p1;
            if (unit01(rng) < config_.crossover_rate) {
                for (int d = 0; d < config_.dimensions; ++d) {
                    const double alpha = unit01(rng);
                    child[static_cast<std::size_t>(d)] = alpha * p1[static_cast<std::size_t>(d)]
                        + (1.0 - alpha) * p2[static_cast<std::size_t>(d)];
                }
            }

            for (int d = 0; d < config_.dimensions; ++d) {
                if (unit01(rng) < config_.mutation_rate) {
                    child[static_cast<std::size_t>(d)] += gaussian(rng);
                }
                child[static_cast<std::size_t>(d)] = clamp_value(
                    child[static_cast<std::size_t>(d)],
                    config_.lower_bound,
                    config_.upper_bound);
            }

            next_population.push_back(child);
        }

        population = std::move(next_population);
        evaluate_population();
    }

    return best_solution_;
}

const std::vector<double>& GeneticSearch::best_solution() const {
    return best_solution_;
}

double GeneticSearch::best_score() const {
    return best_score_;
}

} // namespace opt
