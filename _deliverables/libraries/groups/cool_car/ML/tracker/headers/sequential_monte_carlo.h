#ifndef PARTICLE_FILTER_H
#define PARTICLE_FILTER_H

#include <vector>
#include <random>
#include <functional>
#include <cstdint>
#include "mytrix_eigen_compat.hpp"
#include "base_filter.h"

struct Particle {
    mytrix::Vector state;
    double weight;
};

class SequentialMonteCarlo : public BaseFilter {
public:
    using MotionModel = std::function<mytrix::Vector(const mytrix::Vector&, const mytrix::Vector&, double, std::mt19937&)>;
    using LogLikelihoodModel = std::function<double(const mytrix::Vector&, const mytrix::Vector&)>;

    SequentialMonteCarlo(int num_particles);
    SequentialMonteCarlo(const std::vector<mytrix::Vector>& initial_states,
                         std::uint32_t seed = std::mt19937::default_seed);
    void setMotionModel(MotionModel model);
    void setLogLikelihoodModel(LogLikelihoodModel model);
    void setResamplingThreshold(double effective_sample_ratio);
    void predict(const mytrix::Vector& control, double dt);
    double effectiveSampleSize() const;

    // Implements BaseFilter interface
    void predict() override;
    void update(const mytrix::Vector& z) override;

    const std::vector<Particle>& getParticles() const;

private:
    int num_particles_;
    std::size_t state_dimension_ = 0;
    std::vector<Particle> particles_;
    std::mt19937 gen_;
    MotionModel motion_model_;
    LogLikelihoodModel likelihood_model_;
    double resampling_threshold_ = 1;
};

#endif // PARTICLE_FILTER_H