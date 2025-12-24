#include <gtest/gtest.h>
#include "ml/distribution/bernoulli_distribution.h"
#include "ml/distribution/binomial_distribution.h"
#include "ml/distribution/categorical_distribution.h"
#include "ml/distribution/exponential_distribution.h"
#include "ml/distribution/gamma_distribution.h"
#include "ml/distribution/inverse_gaussian_distribution.h"
#include "ml/distribution/laplace_distribution.h"
#include "ml/distribution/multinomial_distribution.h"
#include "ml/distribution/normal_distribution.h"
#include "ml/distribution/poisson_distribution.h"
#include <cmath>
#include <limits>

// ---------------------------- Bernoulli ----------------------------
TEST(BernoulliDistributionTest, PdfCdf) {
    BernoulliDistribution dist(0.3);
    EXPECT_DOUBLE_EQ(dist.pdf(1.0), 0.3);
    EXPECT_DOUBLE_EQ(dist.pdf(0.0), 0.7);
    EXPECT_DOUBLE_EQ(dist.cdf(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(dist.cdf(0.0), 0.7);
    EXPECT_DOUBLE_EQ(dist.cdf(1.0), 1.0);
    EXPECT_DOUBLE_EQ(dist.log_cdf(1.0), std::log(1.0));
    EXPECT_DOUBLE_EQ(dist.log_pdf(1.0), std::log(0.3));
}

// ---------------------------- Binomial ----------------------------
TEST(BinomialDistributionTest, PdfLogPdfCdf) {
    BinomialDistribution dist(5, 0.5);
    EXPECT_NEAR(dist.pdf(2.0), 0.3125, 1e-6);
    EXPECT_NEAR(std::exp(dist.log_pdf(2.0)), 0.3125, 1e-6);
    EXPECT_GE(dist.cdf(2.0), 0.0);
    EXPECT_LE(dist.cdf(2.0), 1.0);
    EXPECT_LE(dist.log_cdf(2.0), 0.0);
}

// ---------------------------- Categorical ----------------------------
TEST(CategoricalDistributionTest, PdfLogPdfCdf) {
    CategoricalDistribution dist({0.1, 0.4, 0.5});
    EXPECT_DOUBLE_EQ(dist.pdf(0.0), 0.1);
    EXPECT_DOUBLE_EQ(dist.pdf(1.0), 0.4);
    EXPECT_DOUBLE_EQ(dist.pdf(2.0), 0.5);
    EXPECT_DOUBLE_EQ(dist.cdf(0.0), 0.1);
    EXPECT_DOUBLE_EQ(dist.cdf(1.0), 0.5);
    EXPECT_DOUBLE_EQ(dist.log_cdf(2.0), std::log(1.0));
}

// ---------------------------- Exponential ----------------------------
TEST(ExponentialDistributionTest, PdfLogPdfCdf) {
    ExponentialDistribution dist(2.0);
    EXPECT_DOUBLE_EQ(dist.pdf(0.0), 2.0);
    EXPECT_DOUBLE_EQ(dist.pdf(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(dist.cdf(0.0), 0.0);
    EXPECT_LT(dist.log_cdf(0.5), 0.0);
}

// ---------------------------- Gamma ----------------------------
TEST(GammaDistributionTest, PdfLogPdfCdf) {
    GammaDistribution dist(2.0, 1.0);
    EXPECT_GT(dist.pdf(1.0), 0.0);
    EXPECT_GT(dist.log_pdf(1.0), -std::numeric_limits<double>::infinity());
    double cdf_val = dist.cdf(1.0);
    EXPECT_GE(cdf_val, 0.0);
    EXPECT_LE(cdf_val, 1.0);
    EXPECT_LE(dist.log_cdf(1.0), 0.0);
}

// ---------------------------- Normal ----------------------------
TEST(NormalDistributionTest, PdfLogPdfCdf) {
    NormalDistribution dist(0.0, 1.0);
    EXPECT_NEAR(dist.pdf(0.0), 0.398942, 1e-6);
    EXPECT_NEAR(std::exp(dist.log_pdf(0.0)), 0.398942, 1e-6);
    EXPECT_DOUBLE_EQ(dist.cdf(0.0), 0.5);
    EXPECT_DOUBLE_EQ(dist.log_cdf(0.0), std::log(0.5));
}

// ---------------------------- Laplace ----------------------------
TEST(LaplaceDistributionTest, PdfLogPdfCdf) {
    LaplaceDistribution dist(0.0, 1.0);
    EXPECT_DOUBLE_EQ(dist.pdf(0.0), 0.5);
    EXPECT_DOUBLE_EQ(dist.cdf(0.0), 0.5);
    EXPECT_LT(dist.log_cdf(0.0), 0.0);
}

// ---------------------------- Poisson ----------------------------
TEST(PoissonDistributionTest, PdfLogPdfCdf) {
    PoissonDistribution dist(2.0);
    EXPECT_NEAR(dist.pdf(0.0), std::exp(-2.0), 1e-6);
    EXPECT_DOUBLE_EQ(dist.log_pdf(0.0), -2.0);
    EXPECT_LE(dist.cdf(2.0), 1.0);
    EXPECT_LE(dist.log_cdf(2.0), 0.0);
}

// ---------------------------- Existence of Sample ----------------------------
TEST(AllDistributionsTest, SampleNonNegative) {
    BernoulliDistribution b(0.5);
    BinomialDistribution bi(5, 0.5);
    ExponentialDistribution e(1.0);
    GammaDistribution g(2.0, 1.0);
    NormalDistribution n(0.0, 1.0);
    LaplaceDistribution l(0.0, 1.0);
    PoissonDistribution p(3.0);

    EXPECT_NO_THROW(b.sample());
    EXPECT_NO_THROW(bi.sample());
    EXPECT_NO_THROW(e.sample());
    EXPECT_NO_THROW(g.sample());
    EXPECT_NO_THROW(n.sample());
    EXPECT_NO_THROW(l.sample());
    EXPECT_NO_THROW(p.sample());
}