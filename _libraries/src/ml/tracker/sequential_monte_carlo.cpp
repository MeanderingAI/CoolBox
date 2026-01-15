#include <numeric>
#include <algorithm>
#include <cmath>
#include "ML/tracker/sequential_monte_carlo.h"

SequentialMonteCarlo::SequentialMonteCarlo(int num_particles)
    : num_particles_(num_particles), particles_(num_particles) {
    // Initialize particles with default state and uniform weights
    for (auto& p : particles_) {
        p.state = Eigen::Vector3d::Zero();
        p.weight = 1.0 / num_particles_;
    }
}
void SequentialMonteCarlo::predict() {
    // Simple motion model: add Gaussian noise to each particle's state
    std::normal_distribution<double> noise_x(0.0, 0.2);
    std::normal_distribution<double> noise_y(0.0, 0.2);
    std::normal_distribution<double> noise_theta(0.0, 0.05);

    for (auto& p : particles_) {
        p.state(0) += noise_x(gen_);
        p.state(1) += noise_y(gen_);
        p.state(2) += noise_theta(gen_);
    }
}

void SequentialMonteCarlo::update(const Eigen::VectorXd& z) {
    // Example measurement update: assume z = [x_meas, y_meas]
    // Simple likelihood: Gaussian on position
    const double sigma = 1.0;
    const double gauss_norm = 1.0 / (2.0 * M_PI * sigma * sigma);

    for (auto& p : particles_) {
        double dx = p.state(0) - z(0);
        double dy = p.state(1) - z(1);
        double likelihood = gauss_norm * std::exp(-(dx*dx + dy*dy) / (2 * sigma * sigma));
        p.weight *= likelihood;
    }

    // Normalize weights
    double weight_sum = std::accumulate(particles_.begin(), particles_.end(), 0.0,
        [](double sum, const Particle& p) { return sum + p.weight; });

    if (weight_sum > 0) {
        for (auto& p : particles_) {
            p.weight /= weight_sum;
        }
    } else {
        // Reinitialize weights if degenerate
        for (auto& p : particles_) {
            p.weight = 1.0 / num_particles_;
        }
    }

    // Resample particles (systematic resampling)
    std::vector<Particle> new_particles(num_particles_);
    std::uniform_real_distribution<double> dist(0.0, 1.0 / num_particles_);
    double r = dist(gen_);
    double c = particles_[0].weight;
    int i = 0;
    for (int m = 0; m < num_particles_; ++m) {
        double U = r + m * (1.0 / num_particles_);
        while (U > c && i < num_particles_ - 1) {
            ++i;
            c += particles_[i].weight;
        }
        new_particles[m] = particles_[i];
        new_particles[m].weight = 1.0 / num_particles_;
    }
    particles_ = std::move(new_particles);
}

const std::vector<Particle>& SequentialMonteCarlo::getParticles() const {
    return particles_;
}