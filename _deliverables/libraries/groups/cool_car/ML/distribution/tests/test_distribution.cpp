#include "tyst_framework.hpp"
#include "normal_distribution.h"
#include "poisson_distribution.h"
#include "stable_kl.h"
#include "matrix_dense.h"
#include <numeric>
#include <vector>
#include <cmath>

static double PI() { return std::acos(-1.0); }

TEST(NormalDistribution, PdfAndCdfAtMean) {
	NormalDistribution d(1.5, 2.0);
	const double mean = 1.5;
	const double stddev = 2.0;
	const double expected_pdf = 1.0 / (stddev * std::sqrt(2.0 * PI()));
	EXPECT_NEAR(d.pdf(mean), expected_pdf, 1e-12);
	EXPECT_NEAR(d.cdf(mean), 0.5, 1e-12);
}

TEST(NormalDistribution, SamplingMean) {
	NormalDistribution d(0.0, 1.0);
	const int N = 1000;
	std::vector<double> samples;
	samples.reserve(N);
	for (int i = 0; i < N; ++i) samples.push_back(d.sample());
	double avg = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
	// For N=1000 with sigma=1, expect sample mean within ~0.1
	EXPECT_NEAR(avg, 0.0, 0.2);
}

TEST(PoissonDistribution, PdfAndCdfAndLinks) {
	PoissonDistribution p(3.0);
	// pdf at k=2
	double k = 2.0;
	double expected_pdf = std::exp(-3.0) * std::pow(3.0, 2.0) / static_cast<double>(2);
	// Use log_pdf to avoid precision surprises
	EXPECT_NEAR(std::log(p.pdf(k)), p.log_pdf(k), 1e-12);

	// cdf should be between pdf(k) and 1
	double c = p.cdf(k);
	EXPECT_GE(c, 0.0);
	EXPECT_LE(c, 1.0);

	// link functions
	EXPECT_EQ(p.link_name(), std::string("log"));
	double eta = p.link_function(3.0);
	EXPECT_NEAR(eta, std::log(3.0), 1e-12);
	EXPECT_NEAR(p.mean_function(eta), 3.0, 1e-12);
}

TEST(StableKL, ExponentialResidualAtZero) {
	const std::vector<double> value =
		ml::stable_kl::expm1_minus_x({0.0, 1e-8, -1e-8});

	EXPECT_DOUBLE_EQ(value[0], 0.0);
	EXPECT_LT(std::abs(value[1]), 1e-15);
	EXPECT_LT(std::abs(value[2]), 1e-15);
}

TEST(StableKL, IdenticalNormalAndLogits) {
	const std::vector<double> mu = {1.0, -2.0};
	const std::vector<double> logvar = {0.25, -0.5};
	const auto normal = ml::stable_kl::normal_kl(
		mu, logvar, mu, logvar);
	for (double value : normal) {
		EXPECT_DOUBLE_EQ(value, 0.0);
	}

	const std::vector<double> logits = {1.0, 2.0, 3.0};
	const std::vector<double> shifted = {101.0, 102.0, 103.0};
	const auto categorical = ml::stable_kl::kl_from_logits(
		logits, shifted, 1, 3);
	EXPECT_NEAR(categorical[0], 0.0, 1e-15);
}

TEST(StableKL, BuiltInMatrixOverloads) {
	mytrix::DenseMatrix mu =
		mytrix::DenseMatrix::Zero(4, 3);
	mytrix::DenseMatrix logvar =
		mytrix::DenseMatrix::Zero(4, 3);

	const auto normal = ml::stable_kl::normal_kl(
		mu, logvar, mu, logvar);
	EXPECT_DOUBLE_EQ(normal.data.sum(), 0.0);

	mytrix::DenseMatrix teacher(1, 3);
	mytrix::DenseMatrix student(1, 3);
	teacher.data << 1.0, 2.0, 3.0;
	student.data << 101.0, 102.0, 103.0;
	const auto categorical =
		ml::stable_kl::kl_from_logits(teacher, student);
	EXPECT_NEAR(categorical(0), 0.0, 1e-15);
}

