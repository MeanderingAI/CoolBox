#include "tyst_framework.hpp"
#include "vae.h"

#include "mytrix_eigen_compat.hpp"
#include <cmath>
#include <numeric>

using namespace ml;
using MatrixD = Eigen::MatrixXd;

static constexpr double TOL = 1e-3;

// ===================================================================
// Helper: generate simple binary-ish data in [0,1]
// ===================================================================
static MatrixD make_data(int n = 100, int dim = 10, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    MatrixD data(n, dim);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < dim; ++j)
            data(i, j) = dist(rng);
    return data;
}

// Two-cluster data: half rows near 0, half near 1
static MatrixD make_clustered_data(int n = 100, int dim = 5, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0, 0.05);
    MatrixD data(n, dim);
    for (int i = 0; i < n; ++i) {
        double center = (i < n / 2) ? 0.2 : 0.8;
        for (int j = 0; j < dim; ++j)
            data(i, j) = std::max(0.0, std::min(1.0, center + noise(rng)));
    }
    return data;
}

// ===================================================================
// Construction tests
// ===================================================================

TEST(VAETest, ConstructionValid) {
    VAE vae(10, 8, 3);
    EXPECT_EQ(vae.input_dim(), 10);
    EXPECT_EQ(vae.hidden_dim(), 8);
    EXPECT_EQ(vae.latent_dim(), 3);
}

TEST(VAETest, ConstructionInvalidThrows) {
    EXPECT_THROW(VAE(0, 8, 3), std::invalid_argument);
    EXPECT_THROW(VAE(10, 0, 3), std::invalid_argument);
    EXPECT_THROW(VAE(10, 8, 0), std::invalid_argument);
}

// ===================================================================
// Encode / Decode shape tests
// ===================================================================

TEST(VAETest, EncodeShape) {
    VAE vae(10, 8, 3);
    MatrixD data = make_data(20, 10);
    MatrixD z = vae.encode(data);
    EXPECT_EQ(z.rows(), 20);
    EXPECT_EQ(z.cols(), 3);
}

TEST(VAETest, EncodeDistributionShape) {
    VAE vae(10, 8, 3);
    MatrixD data = make_data(20, 10);
    auto [mu, logvar] = vae.encode_distribution(data);
    EXPECT_EQ(mu.rows(), 20);
    EXPECT_EQ(mu.cols(), 3);
    EXPECT_EQ(logvar.rows(), 20);
    EXPECT_EQ(logvar.cols(), 3);
}

TEST(VAETest, DecodeShape) {
    VAE vae(10, 8, 3);
    MatrixD z(5, 3);
    z.setRandom();
    MatrixD x_hat = vae.decode(z);
    EXPECT_EQ(x_hat.rows(), 5);
    EXPECT_EQ(x_hat.cols(), 10);
}

TEST(VAETest, DecodeOutputRange) {
    // Sigmoid output should be in (0, 1)
    VAE vae(10, 8, 3);
    MatrixD z(50, 3);
    z.setRandom();
    MatrixD x_hat = vae.decode(z);
    EXPECT_TRUE((x_hat.array() >= 0.0).all());
    EXPECT_TRUE((x_hat.array() <= 1.0).all());
}

TEST(VAETest, ReconstructShape) {
    VAE vae(10, 8, 3);
    MatrixD data = make_data(15, 10);
    MatrixD recon = vae.reconstruct(data);
    EXPECT_EQ(recon.rows(), 15);
    EXPECT_EQ(recon.cols(), 10);
}

// ===================================================================
// Sample tests
// ===================================================================

TEST(VAETest, SampleShape) {
    VAE vae(10, 8, 3);
    MatrixD samples = vae.sample(20);
    EXPECT_EQ(samples.rows(), 20);
    EXPECT_EQ(samples.cols(), 10);
}

TEST(VAETest, SampleOutputRange) {
    VAE vae(10, 8, 3);
    MatrixD samples = vae.sample(50);
    EXPECT_TRUE((samples.array() >= 0.0).all());
    EXPECT_TRUE((samples.array() <= 1.0).all());
}

// ===================================================================
// Training tests
// ===================================================================

TEST(VAETest, FitReducesLoss) {
    VAE vae(10, 16, 3, 0.001, 42);
    MatrixD data = make_data(100, 10, 42);

    // Loss before training
    double loss_before = -vae.elbo(data);

    auto history = vae.fit(data, 50);

    // Loss after training
    double loss_after = -vae.elbo(data);

    EXPECT_LT(loss_after, loss_before)
        << "Total loss should decrease after training";
}

TEST(VAETest, FitHistoryDecreasing) {
    VAE vae(10, 16, 3, 0.001, 42);
    MatrixD data = make_data(100, 10, 42);

    auto history = vae.fit(data, 30);

    // Check that loss generally decreases (compare first and last)
    EXPECT_LT(history.back().total_loss, history.front().total_loss);
}

TEST(VAETest, FitMiniBatch) {
    VAE vae(10, 16, 3, 0.001, 42);
    MatrixD data = make_data(100, 10, 42);

    double loss_before = -vae.elbo(data);
    vae.fit(data, 50, 32);
    double loss_after = -vae.elbo(data);

    EXPECT_LT(loss_after, loss_before);
}

TEST(VAETest, FitDimensionMismatchThrows) {
    VAE vae(10, 8, 3);
    MatrixD bad_data(20, 5);  // Wrong dimension
    EXPECT_THROW(vae.fit(bad_data), std::invalid_argument);
}

// ===================================================================
// Latent space tests
// ===================================================================

TEST(VAETest, LatentSpaceClusterSeparation) {
    VAE vae(5, 16, 2, 0.005, 42);
    MatrixD data = make_clustered_data(200, 5, 42);

    vae.fit(data, 100);

    // Encode both clusters
    MatrixD z = vae.encode(data);

    // Compute mean latent for each cluster
    Eigen::RowVectorXd mean1 = z.topRows(100).colwise().mean();
    Eigen::RowVectorXd mean2 = z.bottomRows(100).colwise().mean();

    // Clusters should be separated in latent space
    double dist = (mean1 - mean2).norm();
    EXPECT_GT(dist, 0.1) << "Latent cluster means should be separated";
}

TEST(VAETest, ReconstructionQuality) {
    VAE vae(5, 16, 3, 0.005, 42);
    MatrixD data = make_clustered_data(100, 5, 42);

    vae.fit(data, 100);

    MatrixD recon = vae.reconstruct(data);

    // Mean absolute error should be reasonable after training
    double mae = (data - recon).array().abs().mean();
    EXPECT_LT(mae, 0.4) << "Reconstruction MAE should be < 0.4 after training";
}

// ===================================================================
// KL divergence properties
// ===================================================================

TEST(VAETest, KLDivergenceNonNegative) {
    VAE vae(10, 8, 3, 0.001, 42);
    MatrixD data = make_data(50, 10);
    auto history = vae.fit(data, 10);

    for (const auto& h : history) {
        EXPECT_GE(h.kl_divergence, 0.0)
            << "KL divergence should be non-negative";
    }
}

// ===================================================================
// Accessor tests
// ===================================================================

TEST(VAETest, SetLearningRate) {
    VAE vae(10, 8, 3);
    vae.set_learning_rate(0.01);
    EXPECT_DOUBLE_EQ(vae.learning_rate(), 0.01);
}

TEST(VAETest, WeightAccess) {
    VAE vae(10, 8, 3);
    EXPECT_EQ(vae.encoder_W1().rows(), 10);
    EXPECT_EQ(vae.encoder_W1().cols(), 8);
    EXPECT_EQ(vae.encoder_W_mu().rows(), 8);
    EXPECT_EQ(vae.encoder_W_mu().cols(), 3);
    EXPECT_EQ(vae.decoder_W2().rows(), 3);
    EXPECT_EQ(vae.decoder_W2().cols(), 8);
    EXPECT_EQ(vae.decoder_W3().rows(), 8);
    EXPECT_EQ(vae.decoder_W3().cols(), 10);
}
