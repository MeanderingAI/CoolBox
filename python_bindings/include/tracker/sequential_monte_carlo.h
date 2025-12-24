#ifndef PARTICLE_FILTER_H
#define PARTICLE_FILTER_H

#include <vector>
#include <random>
#include <Eigen/Dense>
#include <base_filter.h>

struct Particle {
    Eigen::Vector3d state; // [x, y, theta]
    double weight;
};

class SequentialMonteCarlo : public BaseFilter {
public:
    SequentialMonteCarlo(int num_particles);

    // Implements BaseFilter interface
    void predict() override;
    void update(const Eigen::VectorXd& z) override;

    const std::vector<Particle>& getParticles() const;

private:
    int num_particles_;
    std::vector<Particle> particles_;
    std::default_random_engine gen_;
};

#endif // PARTICLE_FILTER_H