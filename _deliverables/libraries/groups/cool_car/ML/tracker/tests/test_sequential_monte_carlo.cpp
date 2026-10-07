#include "tyst_framework.hpp"

#include "sequential_monte_carlo.h"

#include <cmath>
#include <limits>
#include <stdexcept>

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

TYST_TEST(SequentialMonteCarloTest, SupportsCustomDimensionAndControlledMotion) {
	SequentialMonteCarlo filter({mytrix::Vector(std::vector<double>{2, 3}, 2)}, 7);
	filter.setMotionModel([](const mytrix::Vector& state, const mytrix::Vector& control, double dt, std::mt19937&) {
		auto next = state;
		next.at(0) += control.at(0) * dt;
		return next;
	});
	filter.predict(mytrix::Vector(std::vector<double>{4}, 1), 0.5);
	TYST_EXPECT_NEAR(filter.getParticles()[0].state.at(0), 4, 1e-12);
	TYST_EXPECT_EQ(filter.getParticles()[0].state.size(), 2u);
}
TYST_TEST(SequentialMonteCarloTest, LogWeightsDoNotUnderflow) {
	SequentialMonteCarlo filter({mytrix::Vector(std::vector<double>{0}, 1), mytrix::Vector(std::vector<double>{1}, 1)}, 9);
	filter.setResamplingThreshold(0);
	filter.setLogLikelihoodModel([](const mytrix::Vector& state, const mytrix::Vector&) { return -10000 - state.at(0); });
	filter.update(mytrix::Vector::Zero(1));
	TYST_EXPECT_NEAR(filter.getParticles()[0].weight, 1.0 / (1.0 + std::exp(-1.0)), 1e-12);
	TYST_EXPECT_NEAR(filter.getParticles()[0].weight + filter.getParticles()[1].weight, 1, 1e-12);
}
TYST_TEST(SequentialMonteCarloTest, RejectsInvalidConfigurationAndMissingModels) {
	TYST_EXPECT_THROW(SequentialMonteCarlo(0), std::invalid_argument);
	SequentialMonteCarlo filter({mytrix::Vector::Zero(2)}, 4);
	TYST_EXPECT_THROW(filter.predict(), std::logic_error);
	TYST_EXPECT_THROW(filter.update(mytrix::Vector::Zero(1)), std::logic_error);
	TYST_EXPECT_THROW(filter.setResamplingThreshold(1.5), std::invalid_argument);
}
TYST_TEST(SequentialMonteCarloTest, SeedMakesModelNoiseReproducible) {
	std::vector<mytrix::Vector> states(8, mytrix::Vector::Zero(4));
	SequentialMonteCarlo first(states, 42), second(states, 42);
	const auto model = [](const mytrix::Vector& state, const mytrix::Vector&, double, std::mt19937& generator) {
		auto next = state;
		next.at(3) += std::normal_distribution<double>(0, 0.1)(generator);
		return next;
	};
	first.setMotionModel(model);
	second.setMotionModel(model);
	first.predict(); second.predict();
	for (std::size_t index = 0; index < states.size(); ++index)
		TYST_EXPECT_NEAR(first.getParticles()[index].state.at(3), second.getParticles()[index].state.at(3), 1e-12);
}
