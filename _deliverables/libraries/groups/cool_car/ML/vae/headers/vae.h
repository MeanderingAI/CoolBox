#pragma once

#include "mytrix_eigen_compat.hpp"
#include "matrix_dense.h"
#include <vector>
#include <random>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <functional>

/**
 * @file vae.h
 * @brief Variational Autoencoder with EM-style training.
 *
 * Architecture:
 *   Encoder: x → h₁ = relu(W₁x + b₁) → μ = W_μh₁ + b_μ, log σ² = W_σh₁ + b_σ
 *   Latent:  z = μ + σ ⊙ ε,  ε ~ N(0, I)  (reparameterisation trick)
 *   Decoder: z → h₂ = relu(W₂z + b₂) → x̂ = sigmoid(W₃h₂ + b₃)
 *
 * Loss (ELBO):
 *   L = E_q[log p(x|z)] - KL(q(z|x) ‖ p(z))
 *     = -BCE(x, x̂) - 0.5 * Σ(1 + log σ² - μ² - σ²)
 *
 * EM Interpretation:
 *   E-step: compute q(z|x) via encoder (posterior approximation)
 *   M-step: update θ = {W₁,b₁,W_μ,b_μ,W_σ,b_σ,W₂,b₂,W₃,b₃} via gradient ascent on ELBO
 *
 * Uses manual backpropagation with Eigen — no external autograd.
 */

namespace ml {

/**
 * @brief Training history entry.
 */
struct VAEEpochResult {
    int epoch;
    double total_loss;
    double reconstruction_loss;
    double kl_divergence;
};

/**
 * @brief Variational Autoencoder with Expectation-Maximization training.
 */
class VAE {
public:
    using MatrixD = mytrix::DenseMatrix;
    using VectorD = mytrix::DenseVector;
    using RowVectorD = mytrix::DenseVector;

private:
    int input_dim_;
    int hidden_dim_;
    int latent_dim_;

    // Encoder weights
    MatrixD W1_, W_mu_, W_logvar_;
    VectorD b1_, b_mu_, b_logvar_;

    // Decoder weights
    MatrixD W2_, W3_;
    VectorD b2_, b3_;

    // Training state
    double learning_rate_;
    std::mt19937 rng_;

    // Cached forward pass (for backprop)
    struct ForwardCache {
        MatrixD x;          // Input
        MatrixD h1;         // Encoder hidden (pre-activation)
        MatrixD h1_act;     // Encoder hidden (post-relu)
        MatrixD mu;         // Mean of q(z|x)
        MatrixD logvar;     // Log-variance of q(z|x)
        MatrixD std;        // exp(0.5 * logvar)
        MatrixD eps;        // Sampled noise
        MatrixD z;          // Latent sample
        MatrixD h2;         // Decoder hidden (pre-activation)
        MatrixD h2_act;     // Decoder hidden (post-relu)
        MatrixD x_hat;      // Reconstruction (post-sigmoid)
    };

public:
    /**
     * @brief Construct a VAE.
     * @param input_dim   Dimensionality of input data.
     * @param hidden_dim  Hidden layer size (encoder & decoder).
     * @param latent_dim  Latent space dimensionality.
     * @param lr          Learning rate.
     * @param seed        RNG seed.
     */
    VAE(int input_dim, int hidden_dim, int latent_dim,
        double lr = 0.001, unsigned seed = 42)
        : input_dim_(input_dim), hidden_dim_(hidden_dim),
          latent_dim_(latent_dim), learning_rate_(lr), rng_(seed)
    {
        if (input_dim < 1 || hidden_dim < 1 || latent_dim < 1)
            throw std::invalid_argument("Dimensions must be >= 1");
        init_weights();
    }

    // ---------------------------------------------------------------
    // Training (EM-style)
    // ---------------------------------------------------------------

    /**
     * @brief Train the VAE using EM-style updates.
     *
     * Each epoch:
     *   E-step: Forward pass through encoder to approximate q(z|x)
     *   M-step: Compute ELBO gradients and update all parameters
     *
     * @param data    N × input_dim data matrix (values in [0,1] for BCE loss).
     * @param epochs  Number of training epochs.
     * @param batch_size  Mini-batch size (0 = full batch).
     * @return Training history.
     */
    std::vector<VAEEpochResult> fit(const MatrixD& data, int epochs = 100,
                                     int batch_size = 0) {
        if (data.cols() != input_dim_)
            throw std::invalid_argument("Data dimension mismatch");

        int n = static_cast<int>(data.rows());
        if (batch_size <= 0 || batch_size > n) batch_size = n;

        std::vector<int> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        std::vector<VAEEpochResult> history;

        for (int epoch = 0; epoch < epochs; ++epoch) {
            std::shuffle(indices.begin(), indices.end(), rng_);

            double epoch_recon = 0, epoch_kl = 0;
            int num_batches = 0;

            for (int start = 0; start < n; start += batch_size) {
                int end = std::min(start + batch_size, n);
                int bs = end - start;

                // Extract batch
                MatrixD batch(bs, input_dim_);
                for (int i = 0; i < bs; ++i)
                    batch.data.row(i) = data.row(indices[start + i]).data;

                // ── E-step: forward pass ──
                ForwardCache cache = forward(batch);

                // ── Compute losses ──
                double recon_loss = reconstruction_loss(batch, cache.x_hat);
                double kl_loss = kl_divergence(cache.mu, cache.logvar);

                epoch_recon += recon_loss * bs;
                epoch_kl += kl_loss * bs;

                // ── M-step: backward pass + parameter update ──
                backward(cache);

                ++num_batches;
            }

            VAEEpochResult result;
            result.epoch = epoch;
            result.reconstruction_loss = epoch_recon / n;
            result.kl_divergence = epoch_kl / n;
            result.total_loss = result.reconstruction_loss + result.kl_divergence;
            history.push_back(result);
        }

        return history;
    }

    // ---------------------------------------------------------------
    // Inference
    // ---------------------------------------------------------------

    /**
     * @brief Encode data to latent space (returns mean of q(z|x)).
     */
    MatrixD encode(const MatrixD& x) const {
        MatrixD h1 = (x * W1_).rowwise_add(b1_);
        MatrixD h1_act = relu(h1);
        MatrixD mu = (h1_act * W_mu_).rowwise_add(b_mu_);
        return mu;
    }

    /**
     * @brief Encode to full posterior parameters (mean and log-variance).
     */
    std::pair<MatrixD, MatrixD> encode_distribution(const MatrixD& x) const {
        MatrixD h1 = (x * W1_).rowwise_add(b1_);
        MatrixD h1_act = relu(h1);
        MatrixD mu = (h1_act * W_mu_).rowwise_add(b_mu_);
        MatrixD logvar = (h1_act * W_logvar_).rowwise_add(b_logvar_);
        return {mu, logvar};
    }

    /**
     * @brief Decode latent vectors to reconstructions.
     */
    MatrixD decode(const MatrixD& z) const {
        MatrixD h2 = (z * W2_).rowwise_add(b2_);
        MatrixD h2_act = relu(h2);
        MatrixD logits = (h2_act * W3_).rowwise_add(b3_);
        return sigmoid(logits);
    }

    /**
     * @brief Full forward pass: encode → sample → decode.
     */
    MatrixD reconstruct(const MatrixD& x) {
        ForwardCache cache = forward(x);
        return cache.x_hat;
    }

    /**
     * @brief Sample from the prior p(z) = N(0, I) and decode.
     * @param n  Number of samples.
     */
    MatrixD sample(int n) {
        MatrixD z = sample_noise(n, latent_dim_);
        return decode(z);
    }

    /**
     * @brief Compute ELBO for data.
     */
    double elbo(const MatrixD& data) {
        ForwardCache cache = forward(data);
        double recon = reconstruction_loss(data, cache.x_hat);
        double kl = kl_divergence(cache.mu, cache.logvar);
        return -(recon + kl);  // ELBO = -loss
    }

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    int input_dim() const { return input_dim_; }
    int hidden_dim() const { return hidden_dim_; }
    int latent_dim() const { return latent_dim_; }
    double learning_rate() const { return learning_rate_; }
    void set_learning_rate(double lr) { learning_rate_ = lr; }

    // Weight access (for inspection / serialisation)
    const MatrixD& encoder_W1() const { return W1_; }
    const MatrixD& encoder_W_mu() const { return W_mu_; }
    const MatrixD& encoder_W_logvar() const { return W_logvar_; }
    const MatrixD& decoder_W2() const { return W2_; }
    const MatrixD& decoder_W3() const { return W3_; }

private:
    // ---------------------------------------------------------------
    // Weight initialisation (Xavier)
    // ---------------------------------------------------------------

    void init_weights() {
        auto xavier = [&](int fan_in, int fan_out) -> MatrixD {
            double scale = std::sqrt(2.0 / (fan_in + fan_out));
            std::normal_distribution<double> dist(0.0, scale);
            MatrixD W(fan_in, fan_out);
            for (int i = 0; i < fan_in; ++i)
                for (int j = 0; j < fan_out; ++j)
                    W(i, j) = dist(rng_);
            return W;
        };

        // Encoder
        W1_      = xavier(input_dim_, hidden_dim_);
        b1_      = VectorD::Zero(hidden_dim_);
        W_mu_    = xavier(hidden_dim_, latent_dim_);
        b_mu_    = VectorD::Zero(latent_dim_);
        W_logvar_ = xavier(hidden_dim_, latent_dim_);
        b_logvar_ = VectorD::Zero(latent_dim_);

        // Decoder
        W2_ = xavier(latent_dim_, hidden_dim_);
        b2_ = VectorD::Zero(hidden_dim_);
        W3_ = xavier(hidden_dim_, input_dim_);
        b3_ = VectorD::Zero(input_dim_);
    }

    // ---------------------------------------------------------------
    // Activation functions
    // ---------------------------------------------------------------

    static MatrixD relu(const MatrixD& x) {
        return MatrixD(x.data.cwiseMax(0.0));
    }

    static MatrixD relu_grad(const MatrixD& x) {
        return MatrixD((x.data.array() > 0.0).cast<double>());
    }

    static MatrixD sigmoid(const MatrixD& x) {
        return MatrixD((1.0 + (-x.data.array()).exp()).inverse().matrix());
    }

    // ---------------------------------------------------------------
    // Forward pass
    // ---------------------------------------------------------------

    ForwardCache forward(const MatrixD& x) {
        ForwardCache c;
        c.x = x;
        int bs = static_cast<int>(x.rows());

        // Encoder
        c.h1     = MatrixD((x.data * W1_.data).rowwise() + b1_.data.transpose());
        c.h1_act = relu(c.h1);
        c.mu     = MatrixD((c.h1_act.data * W_mu_.data).rowwise() + b_mu_.data.transpose());
        c.logvar = MatrixD((c.h1_act.data * W_logvar_.data).rowwise() + b_logvar_.data.transpose());

        // Reparameterisation: z = μ + σ ⊙ ε
        c.std = MatrixD((0.5 * c.logvar.data.array()).exp().matrix());
        c.eps = sample_noise(bs, latent_dim_);
        c.z   = MatrixD(c.mu.data + c.std.data.cwiseProduct(c.eps.data));

        // Decoder
        c.h2     = MatrixD((c.z.data * W2_.data).rowwise() + b2_.data.transpose());
        c.h2_act = relu(c.h2);
        MatrixD logits = MatrixD((c.h2_act.data * W3_.data).rowwise() + b3_.data.transpose());
        c.x_hat = sigmoid(logits);

        return c;
    }

    // ---------------------------------------------------------------
    // Loss functions
    // ---------------------------------------------------------------

    /** @brief Binary cross-entropy (mean over batch). */
    static double reconstruction_loss(const MatrixD& x, const MatrixD& x_hat) {
        // Clamp x_hat to avoid log(0)
        MatrixD xh = MatrixD(x_hat.data.array().max(1e-8).min(1.0 - 1e-8).matrix());
        double bce = -(x.data.array() * xh.data.array().log()
                 + (1.0 - x.data.array()) * (1.0 - xh.data.array()).log()).sum();
        return bce / x.data.rows();
    }

    /** @brief KL divergence KL(q(z|x) ‖ p(z)) (mean over batch). */
    static double kl_divergence(const MatrixD& mu, const MatrixD& logvar) {
        // KL = -0.5 * Σ(1 + log σ² - μ² - σ²)
        double kl = -0.5 * (1.0 + logvar.data.array() - mu.data.array().square()
                  - logvar.data.array().exp()).sum();
        return kl / mu.data.rows();
    }

    // ---------------------------------------------------------------
    // Backward pass (manual gradients)
    // ---------------------------------------------------------------

    void backward(const ForwardCache& c) {
        int bs = static_cast<int>(c.x.rows());
        double inv_bs = 1.0 / bs;

        // ── Decoder gradients ──

        // d_loss / d_logits = x_hat - x  (BCE gradient through sigmoid)
        MatrixD d_logits = MatrixD(c.x_hat.data - c.x.data);

        // Gradients for W3, b3
        MatrixD dW3 = MatrixD(c.h2_act.data.transpose() * d_logits.data * inv_bs);
        VectorD db3 = VectorD((d_logits.data.colwise().mean()).transpose());

        // Backprop through relu
        MatrixD d_h2_act = MatrixD(d_logits.data * W3_.data.transpose());
        MatrixD d_h2 = MatrixD(d_h2_act.data.cwiseProduct(relu_grad(c.h2).data));

        // Gradients for W2, b2
        MatrixD dW2 = MatrixD(c.z.data.transpose() * d_h2.data * inv_bs);
        VectorD db2 = VectorD((d_h2.data.colwise().mean()).transpose());

        // ── Latent gradients ──

        // d_loss / d_z (from reconstruction)
        MatrixD d_z = MatrixD(d_h2.data * W2_.data.transpose());

        // ── KL gradients (added to encoder gradients) ──
        // d_KL / d_mu = mu
        // d_KL / d_logvar = 0.5 * (exp(logvar) - 1)
        MatrixD d_mu_kl = MatrixD(c.mu.data * inv_bs);
        MatrixD d_logvar_kl = MatrixD(0.5 * (c.logvar.data.array().exp() - 1.0).matrix() * inv_bs);

        // ── Encoder gradients (through reparameterisation) ──

        // d_z / d_mu = I
        // d_z / d_std = eps
        // d_z / d_logvar = 0.5 * std * eps  (chain through std = exp(0.5*logvar))
        MatrixD d_mu = MatrixD(d_z.data + d_mu_kl.data);
        MatrixD d_logvar = MatrixD(d_z.data.cwiseProduct(c.eps.data).cwiseProduct(c.std.data) * 0.5 + d_logvar_kl.data);

        // Gradients for W_mu, b_mu
        MatrixD dW_mu = MatrixD(c.h1_act.data.transpose() * d_mu.data * inv_bs);
        VectorD db_mu = VectorD((d_mu.data.colwise().mean()).transpose());

        // Gradients for W_logvar, b_logvar
        MatrixD dW_logvar = MatrixD(c.h1_act.data.transpose() * d_logvar.data * inv_bs);
        VectorD db_logvar = VectorD((d_logvar.data.colwise().mean()).transpose());

        // Backprop through encoder hidden
        MatrixD d_h1_act = MatrixD(d_mu.data * W_mu_.data.transpose()
                 + d_logvar.data * W_logvar_.data.transpose());
        MatrixD d_h1 = MatrixD(d_h1_act.data.cwiseProduct(relu_grad(c.h1).data));

        // Gradients for W1, b1
        MatrixD dW1 = MatrixD(c.x.data.transpose() * d_h1.data * inv_bs);
        VectorD db1 = VectorD((d_h1.data.colwise().mean()).transpose());

        // ── M-step: gradient descent update ──
        W1_       = MatrixD(W1_.data - dW1.data * learning_rate_);
        b1_       = VectorD(b1_.data - db1.data * learning_rate_);
        W_mu_     = MatrixD(W_mu_.data - dW_mu.data * learning_rate_);
        b_mu_     = VectorD(b_mu_.data - db_mu.data * learning_rate_);
        W_logvar_ = MatrixD(W_logvar_.data - dW_logvar.data * learning_rate_);
        b_logvar_ = VectorD(b_logvar_.data - db_logvar.data * learning_rate_);
        W2_       = MatrixD(W2_.data - dW2.data * learning_rate_);
        b2_       = VectorD(b2_.data - db2.data * learning_rate_);
        W3_       = MatrixD(W3_.data - dW3.data * learning_rate_);
        b3_       = VectorD(b3_.data - db3.data * learning_rate_);
    }

    // ---------------------------------------------------------------
    // Sampling
    // ---------------------------------------------------------------

    MatrixD sample_noise(int rows, int cols) {
        std::normal_distribution<double> dist(0.0, 1.0);
        MatrixD noise(rows, cols);
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                noise(i, j) = dist(rng_);
        return noise;
    }
};

} // namespace ml
