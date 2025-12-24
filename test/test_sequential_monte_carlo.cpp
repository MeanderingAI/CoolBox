#include <gtest/gtest.h>
#include "ml/tracker/sequential_monte_carlo.h"

// Test construction and initial weights
TEST(SequentialMonteCarloTest, Initialization) {
    SequentialMonteCarlo pf(100);
    const auto& particles = pf.getParticles();
    ASSERT_EQ(particles.size(), 100);
    for (const auto& p : particles) {
        EXPECT_DOUBLE_EQ(p.weight, 1.0 / 100);
        EXPECT_EQ(p.state, Eigen::Vector3d::Zero());
    }
}

// Test predict step adds noise
TEST(SequentialMonteCarloTest, PredictAddsNoise) {
    SequentialMonteCarlo pf(10);
    auto before = pf.getParticles();
    pf.predict();
    const auto& after = pf.getParticles();
    bool changed = false;
    for (size_t i = 0; i < before.size(); ++i) {
        if (!before[i].state.isApprox(after[i].state)) {
            changed = true;
            break;
        }
    }
    EXPECT_TRUE(changed);
}

// Test update step normalizes weights and resamples
TEST(SequentialMonteCarloTest, UpdateNormalizesWeightsAndResamples) {
    SequentialMonteCarlo pf(50);
    Eigen::Vector2d z(1.0, 2.0);
    pf.update(z);
    const auto& particles = pf.getParticles();
    double sum = 0.0;
    for (const auto& p : particles) {
        sum += p.weight;
    }
    EXPECT_NEAR(sum, 1.0, 1e-6);

    // After resampling, all weights should be equal
    for (const auto& p : particles) {
        EXPECT_NEAR(p.weight, 1.0 / 50, 1e-9);
    }
}

// Test degenerate case: all weights zero
TEST(SequentialMonteCarloTest, DegenerateWeightsReinitialized) {
    SequentialMonteCarlo pf(20);
    auto& particles = const_cast<std::vector<Particle>&>(pf.getParticles());
    for (auto& p : particles) {
        p.weight = 0.0;
    }
    Eigen::Vector2d z(0.0, 0.0);
    pf.update(z);
    for (const auto& p : pf.getParticles()) {
        EXPECT_DOUBLE_EQ(p.weight, 1.0 / 20);
    }
}