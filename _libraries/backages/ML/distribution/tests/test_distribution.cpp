#include <gtest/gtest.h>
#include "normal_distribution.h"
#include "poisson_distribution.h"
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

