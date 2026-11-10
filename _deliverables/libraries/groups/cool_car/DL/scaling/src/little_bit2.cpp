#include "little_bit2.h"

#include "samsung_quantization_internal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

double at(const Tensor& x, size_t row, size_t col) {
    return x.data()[row * x.shape()[1] + col];
}
double& at(Tensor& x, size_t row, size_t col) {
    return x.data()[row * x.shape()[1] + col];
}

Tensor orthonormalize(Tensor result) {
    const size_t rank = result.shape()[0];
    for (size_t column = 0; column < rank; ++column) {
        for (size_t previous = 0; previous < column; ++previous) {
            double dot = 0.0;
            for (size_t row = 0; row < rank; ++row)
                dot += at(result, row, column) * at(result, row, previous);
            for (size_t row = 0; row < rank; ++row)
                at(result, row, column) -= dot * at(result, row, previous);
        }
        double norm = 0.0;
        for (size_t row = 0; row < rank; ++row) norm += at(result, row, column) * at(result, row, column);
        norm = std::sqrt(norm);
        if (norm < 1e-14) {
            // Choose the least-aligned coordinate and orthogonalize it too.
            size_t coordinate = 0;
            double least = std::numeric_limits<double>::infinity();
            for (size_t candidate = 0; candidate < rank; ++candidate) {
                double alignment = 0.0;
                for (size_t previous = 0; previous < column; ++previous)
                    alignment += std::abs(at(result, candidate, previous));
                if (alignment < least) {
                    least = alignment;
                    coordinate = candidate;
                }
            }
            for (size_t row = 0; row < rank; ++row)
                at(result, row, column) = row == coordinate ? 1.0 : 0.0;
            for (size_t previous = 0; previous < column; ++previous) {
                double dot = 0.0;
                for (size_t row = 0; row < rank; ++row)
                    dot += at(result, row, column) * at(result, row, previous);
                for (size_t row = 0; row < rank; ++row)
                    at(result, row, column) -= dot * at(result, row, previous);
            }
            norm = 0.0;
            for (size_t row = 0; row < rank; ++row)
                norm += at(result, row, column) * at(result, row, column);
            norm = std::sqrt(norm);
            for (size_t row = 0; row < rank; ++row) at(result, row, column) /= norm;
        } else {
            for (size_t row = 0; row < rank; ++row) at(result, row, column) /= norm;
        }
    }
    return result;
}

Tensor random_orthogonal(size_t rank, uint32_t seed) {
    std::mt19937 generator(seed);
    std::normal_distribution<double> normal(0.0, 1.0);
    Tensor result({rank, rank}, 0.0);
    for (double& value : result.data()) value = normal(generator);
    return orthonormalize(std::move(result));
}

std::pair<Tensor, Tensor> initial_factors(const Tensor& weights, size_t rank) {
    const auto svd = samsung_internal::truncated_svd(weights, rank);
    Tensor u({weights.shape()[0], rank}, 0.0);
    Tensor v({rank, weights.shape()[1]}, 0.0);
    for (size_t k = 0; k < rank; ++k) {
        const double scale = std::sqrt(svd.singular_values[k]);
        for (size_t i = 0; i < weights.shape()[0]; ++i) at(u, i, k) = at(svd.u, i, k) * scale;
        for (size_t j = 0; j < weights.shape()[1]; ++j) at(v, k, j) = at(svd.vt, k, j) * scale;
    }
    return {u, v};
}

Tensor compute_joint_itq(const Tensor& x, size_t iterations, uint32_t seed) {
    const size_t rank = x.shape()[1];
    Tensor rotation = random_orthogonal(rank, seed);
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        Tensor binary = samsung_internal::sign(x.matmul(rotation));
        Tensor cross = binary.transpose().matmul(x);
        const auto svd = samsung_internal::truncated_svd(cross, rank);
        rotation = orthonormalize(svd.vt.transpose().matmul(svd.u.transpose()));
    }
    return rotation;
}

std::pair<Tensor, Tensor> scale_pair(const Tensor& factor) {
    Tensor magnitude(factor.shape(), 0.0);
    for (size_t i = 0; i < factor.size(); ++i) magnitude.data()[i] = std::abs(factor.data()[i]);
    const auto svd = samsung_internal::truncated_svd(magnitude, 1);
    const double scale = std::sqrt(svd.singular_values[0]);
    Tensor left({factor.shape()[0]}, 0.0), right({factor.shape()[1]}, 0.0);
    for (size_t i = 0; i < left.size(); ++i) left.data()[i] = std::abs(at(svd.u, i, 0)) * scale;
    for (size_t j = 0; j < right.size(); ++j) right.data()[j] = std::abs(at(svd.vt, 0, j)) * scale;
    return {left, right};
}

LittleBitFactors deploy(const Tensor& u, const Tensor& v) {
    auto us = scale_pair(u);
    auto vs = scale_pair(v);
    Tensor latent({u.shape()[1]}, 0.0);
    for (size_t k = 0; k < latent.size(); ++k)
        latent.data()[k] = us.second.data()[k] * vs.first.data()[k];
    return {pack_binary_signs(u), pack_binary_signs(v), us.first, latent, vs.second};
}

LittleBitFactors aligned_path(const Tensor& weights, const LittleBit2Config& config, uint32_t seed) {
    auto factors = initial_factors(weights, config.rank);
    Tensor combined({weights.shape()[0] + weights.shape()[1], config.rank}, 0.0);
    for (size_t i = 0; i < weights.shape()[0]; ++i)
        for (size_t k = 0; k < config.rank; ++k) at(combined, i, k) = at(factors.first, i, k);
    for (size_t j = 0; j < weights.shape()[1]; ++j)
        for (size_t k = 0; k < config.rank; ++k)
            at(combined, weights.shape()[0] + j, k) = at(factors.second, k, j);
    const Tensor rotation = compute_joint_itq(combined, config.alignment_iterations, seed);
    Tensor u = factors.first.matmul(rotation);
    Tensor v = rotation.transpose().matmul(factors.second);
    return deploy(u, v);
}

} // namespace

Tensor joint_itq_rotation(const Tensor& latent_rows, size_t iterations, uint32_t seed) {
    samsung_internal::validate_matrix(latent_rows, "joint_itq_rotation");
    if (iterations == 0)
        throw std::invalid_argument("joint_itq_rotation: iterations must be > 0");
    return compute_joint_itq(latent_rows, iterations, seed);
}

double latent_alignment_error(const Tensor& latent_rows, const Tensor& rotation) {
    samsung_internal::validate_matrix(latent_rows, "latent_alignment_error");
    samsung_internal::validate_matrix(rotation, "latent_alignment_error");
    if (rotation.shape()[0] != rotation.shape()[1] ||
        latent_rows.shape()[1] != rotation.shape()[0])
        throw std::invalid_argument("latent_alignment_error: incompatible shapes");
    const Tensor rotated = latent_rows.matmul(rotation);
    return samsung_internal::squared_error(samsung_internal::sign(rotated), rotated);
}

LittleBitResult little_bit2_quantize(const Tensor& weights, const LittleBit2Config& config) {
    samsung_internal::validate_matrix(weights, "little_bit2_quantize");
    if (config.rank == 0 || config.rank > std::min(weights.shape()[0], weights.shape()[1]))
        throw std::invalid_argument("little_bit2_quantize: invalid rank");
    if (config.alignment_iterations == 0)
        throw std::invalid_argument("little_bit2_quantize: alignment_iterations must be > 0");

    LittleBitResult result;
    result.primary = aligned_path(weights, config, config.seed);
    if (config.residual_compensation) {
        result.residual = aligned_path(weights - result.primary.reconstruct(), config, config.seed + 1u);
        result.has_residual = true;
    }
    return result;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
