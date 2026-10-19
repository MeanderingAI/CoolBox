#include "sequential_monte_carlo.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
void validateState(const mytrix::Vector& state, std::size_t dimension) {
    if (state.size() != dimension) throw std::invalid_argument("Particle state dimension mismatch");
    for (std::size_t index = 0; index < dimension; ++index)
        if (!std::isfinite(state.at(index))) throw std::invalid_argument("Particle state must be finite");
}
std::vector<mytrix::Vector> legacyStates(int count) {
    if (count <= 0) throw std::invalid_argument("Particle count must be positive");
    std::mt19937 generator(std::random_device{}());
    std::normal_distribution<double> distribution(0, 1);
    std::vector<mytrix::Vector> states;
    for (int particle = 0; particle < count; ++particle)
        states.emplace_back(std::vector<double>{distribution(generator), distribution(generator), distribution(generator)}, 3);
    return states;
}
}

SequentialMonteCarlo::SequentialMonteCarlo(int num_particles)
    : SequentialMonteCarlo(legacyStates(num_particles), std::random_device{}()) {
    setMotionModel([](const mytrix::Vector& state, const mytrix::Vector&, double dt, std::mt19937& generator) {
        auto next = state;
        std::normal_distribution<double> noise(0, 0.1 * std::sqrt(dt));
        next.at(0) += std::cos(state.at(2)) * 0.1 * dt + noise(generator);
        next.at(1) += std::sin(state.at(2)) * 0.1 * dt + noise(generator);
        next.at(2) += noise(generator) * 0.05;
        return next;
    });
    setLogLikelihoodModel([](const mytrix::Vector& state, const mytrix::Vector& measurement) {
        if (measurement.size() > state.size()) throw std::invalid_argument("Measurement exceeds legacy state dimension");
        double squaredDistance = 0;
        for (std::size_t index = 0; index < measurement.size(); ++index) {
            const double difference = measurement.at(index) - state.at(index);
            squaredDistance += difference * difference;
        }
        return -0.5 * squaredDistance;
    });
}

SequentialMonteCarlo::SequentialMonteCarlo(const std::vector<mytrix::Vector>& states, std::uint32_t seed)
    : num_particles_(static_cast<int>(states.size())), gen_(seed) {
    if (states.empty() || states.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) || states.front().size() == 0)
        throw std::invalid_argument("Nonempty particle states are required");
    state_dimension_ = states.front().size();
    for (const auto& state : states) {
        validateState(state, state_dimension_);
        particles_.push_back({state, 1.0 / num_particles_});
    }
}
void SequentialMonteCarlo::setMotionModel(MotionModel model) {
    if (!model) throw std::invalid_argument("Motion model is required");
    motion_model_ = std::move(model);
}
void SequentialMonteCarlo::setLogLikelihoodModel(LogLikelihoodModel model) {
    if (!model) throw std::invalid_argument("Likelihood model is required");
    likelihood_model_ = std::move(model);
}
void SequentialMonteCarlo::setResamplingThreshold(double ratio) {
    if (!std::isfinite(ratio) || ratio < 0 || ratio > 1) throw std::invalid_argument("Resampling ratio must be between zero and one");
    resampling_threshold_ = ratio;
}
void SequentialMonteCarlo::predict() { predict(mytrix::Vector::Zero(0), 1); }
void SequentialMonteCarlo::predict(const mytrix::Vector& control, double dt) {
    if (!motion_model_) throw std::logic_error("Configure a particle motion model before prediction");
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Prediction interval must be finite and nonnegative");
    validateState(control, control.size());
    if (dt == 0) return;
    auto predicted = particles_;
    for (auto& particle : predicted) {
        auto state = motion_model_(particle.state, control, dt, gen_);
        validateState(state, state_dimension_);
        particle.state = std::move(state);
    }
    particles_ = std::move(predicted);
}
double SequentialMonteCarlo::effectiveSampleSize() const {
    double squaredWeights = 0;
    for (const auto& particle : particles_) squaredWeights += particle.weight * particle.weight;
    return squaredWeights > 0 ? 1 / squaredWeights : 0;
}
void SequentialMonteCarlo::update(const mytrix::Vector& measurement) {
    if (!likelihood_model_) throw std::logic_error("Configure a particle likelihood model before updating");
    validateState(measurement, measurement.size());
    std::vector<double> logWeights;
    double maximum = -std::numeric_limits<double>::infinity();
    for (const auto& particle : particles_) {
        const double likelihood = likelihood_model_(particle.state, measurement);
        if (std::isnan(likelihood) || likelihood == std::numeric_limits<double>::infinity()) throw std::invalid_argument("Invalid particle log likelihood");
        const double weight = particle.weight > 0 ? std::log(particle.weight) + likelihood : -std::numeric_limits<double>::infinity();
        logWeights.push_back(weight);
        maximum = std::max(maximum, weight);
    }
    double total = 0;
    std::vector<double> weights(logWeights.size(), 1);
    if (std::isfinite(maximum)) {
        for (std::size_t index = 0; index < weights.size(); ++index) {
            weights[index] = std::exp(logWeights[index] - maximum);
            total += weights[index];
        }
    } else total = static_cast<double>(weights.size());
    for (std::size_t index = 0; index < weights.size(); ++index) particles_[index].weight = weights[index] / total;
    if (resampling_threshold_ == 0 || effectiveSampleSize() > resampling_threshold_ * num_particles_) return;
    std::vector<double> cumulative(num_particles_);
    cumulative[0] = particles_[0].weight;
    for (int index = 1; index < num_particles_; ++index) cumulative[index] = cumulative[index - 1] + particles_[index].weight;
    cumulative.back() = 1;
    std::uniform_real_distribution<double> distribution(0, 1.0 / num_particles_);
    const double offset = distribution(gen_);
    std::vector<Particle> resampled;
    int ancestor = 0;
    for (int index = 0; index < num_particles_; ++index) {
        const double target = offset + static_cast<double>(index) / num_particles_;
        while (ancestor < num_particles_ - 1 && cumulative[ancestor] < target) ++ancestor;
        resampled.push_back({particles_[ancestor].state, 1.0 / num_particles_});
    }
    particles_ = std::move(resampled);
}
const std::vector<Particle>& SequentialMonteCarlo::getParticles() const { return particles_; }