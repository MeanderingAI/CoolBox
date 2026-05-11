#include "sequential_monte_carlo.h"
#include <algorithm>
#include <numeric>
#include <cmath>

SequentialMonteCarlo::SequentialMonteCarlo(int num_particles)
    : num_particles_(num_particles) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0, 1.0);
    
    particles_.resize(num_particles_);
    double uniform_weight = 1.0 / num_particles_;
    for (auto& p : particles_) {
        p.state = mytrix::Vector{dist(gen), dist(gen), dist(gen)};
        p.weight = uniform_weight;
    }
}

void SequentialMonteCarlo::predict() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> noise(0.0, 0.1);
    
    for (auto& p : particles_) {
        // Simple random walk model
        p.state[0] += std::cos(p.state[2]) * 0.1 + noise(gen);
        p.state[1] += std::sin(p.state[2]) * 0.1 + noise(gen);
        p.state[2] += noise(gen) * 0.05;
    }
}

void SequentialMonteCarlo::update(const mytrix::Vector& z) {
    // Update weights based on measurement likelihood
    double total_weight = 0.0;
    for (auto& p : particles_) {
        // Simple Gaussian likelihood
        mytrix::Vector expected_z(p.state.begin(), p.state.begin() + z.size());
        double dist_sq = 0.0;
        for (size_t i = 0; i < z.size(); ++i) {
            double diff = z[i] - expected_z[i];
            dist_sq += diff * diff;
        }
        p.weight *= std::exp(-0.5 * dist_sq);
        total_weight += p.weight;
    }
    
    // Normalize weights
    if (total_weight > 1e-15) {
        for (auto& p : particles_) {
            p.weight /= total_weight;
        }
    } else {
        double uniform = 1.0 / num_particles_;
        for (auto& p : particles_) p.weight = uniform;
    }
    
    // Resample using systematic resampling
    std::vector<Particle> new_particles(num_particles_);
    std::vector<double> cumsum(num_particles_);
    cumsum[0] = particles_[0].weight;
    for (int i = 1; i < num_particles_; ++i) {
        cumsum[i] = cumsum[i - 1] + particles_[i].weight;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> uniform(0.0, 1.0 / num_particles_);
    double u = uniform(gen);
    
    int j = 0;
    for (int i = 0; i < num_particles_; ++i) {
        double target = u + static_cast<double>(i) / num_particles_;
        while (j < num_particles_ - 1 && cumsum[j] < target) j++;
        new_particles[i] = particles_[j];
        new_particles[i].weight = 1.0 / num_particles_;
    }
    particles_ = new_particles;
}

const std::vector<Particle>& SequentialMonteCarlo::getParticles() const {
    return particles_;
}
