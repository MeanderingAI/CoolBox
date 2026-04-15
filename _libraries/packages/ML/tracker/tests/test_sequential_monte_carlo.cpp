#include "tyst_framework.hpp"

#include "sequential_monte_carlo.h"

#include <cmath>

TYST_TEST(SequentialMonteCarloTest, NormalizesWeightsAfterUpdate) {
	SequentialMonteCarlo filter(50);
	filter.predict();

	Eigen::VectorXd measurement(2);
	measurement << 0.0, 0.0;
	filter.update(measurement);

	const auto& particles = filter.getParticles();
	double weight_sum = 0.0;
	for (const auto& particle : particles) {
		TYST_EXPECT_TRUE(std::isfinite(particle.state(0)));
		TYST_EXPECT_TRUE(std::isfinite(particle.state(1)));
		TYST_EXPECT_TRUE(std::isfinite(particle.state(2)));
		weight_sum += particle.weight;
		TYST_EXPECT_NEAR(particle.weight, 1.0 / 50.0, 1e-12);
	}

	TYST_EXPECT_EQ(particles.size(), 50u);
	TYST_EXPECT_NEAR(weight_sum, 1.0, 1e-12);
}
