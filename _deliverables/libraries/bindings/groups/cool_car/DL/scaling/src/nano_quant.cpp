#include "nano_quant.h"

#include "samsung_quantization_internal.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

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

Tensor admm_least_squares(const Tensor& x, const Tensor& y, const Tensor& z,
                          const Tensor& dual, double rho, double regularization) {
    Tensor system = x.transpose().matmul(x);
    double diagonal_mean = 0.0;
    for (size_t i = 0; i < system.shape()[0]; ++i) diagonal_mean += std::abs(at(system, i, i));
    diagonal_mean /= static_cast<double>(system.shape()[0]);
    const double stabilizer = rho * diagonal_mean + regularization;
    for (size_t i = 0; i < system.shape()[0]; ++i) at(system, i, i) += stabilizer;
    Tensor rhs = x.transpose().matmul(y);
    for (size_t i = 0; i < rhs.size(); ++i)
        rhs.data()[i] += rho * (z.data()[i] - dual.data()[i]);
    return samsung_internal::solve(system, rhs);
}

void validate_importance(const Tensor& importance, size_t size, const char* name) {
    if (importance.size() == 0) return;
    if (importance.shape() != std::vector<size_t>{size})
        throw std::invalid_argument(std::string("nano_quantize: ") + name + " has the wrong shape");
    for (double value : importance.data()) {
        if (!std::isfinite(value) || value < 0.0)
            throw std::invalid_argument(std::string("nano_quantize: ") + name +
                                        " must be finite and non-negative");
    }
}

double frobenius_norm(const Tensor& tensor) {
    double result = 0.0;
    for (double value : tensor.data()) result += value * value;
    return std::sqrt(result);
}

} // namespace

size_t NanoQuantFactors::logical_storage_bits() const {
    return u_sign.logical_bits() + v_sign.logical_bits() +
           16 * (pre_scale.size() + post_scale.size());
}

size_t NanoQuantFactors::packed_storage_bits() const {
    return u_sign.packed_bits() + v_sign.packed_bits() +
           16 * (pre_scale.size() + post_scale.size());
}

double NanoQuantFactors::bits_per_weight() const {
    const size_t count = u_sign.rows * v_sign.cols;
    return count == 0 ? 0.0 :
        static_cast<double>(logical_storage_bits()) / static_cast<double>(count);
}

Tensor NanoQuantFactors::reconstruct() const {
    if (u_sign.cols != v_sign.rows ||
        pre_scale.shape() != std::vector<size_t>{v_sign.cols} ||
        post_scale.shape() != std::vector<size_t>{u_sign.rows})
        throw std::invalid_argument("NanoQuantFactors::reconstruct: inconsistent factor shapes");
    Tensor result({u_sign.rows, v_sign.cols}, 0.0);
    for (size_t out = 0; out < u_sign.rows; ++out) {
        for (size_t in = 0; in < v_sign.cols; ++in) {
            double sum = 0.0;
            for (size_t k = 0; k < u_sign.cols; ++k)
                sum += static_cast<double>(u_sign.value(out, k) * v_sign.value(k, in));
            result.data()[out * v_sign.cols + in] =
                post_scale.data()[out] * sum * pre_scale.data()[in];
        }
    }
    return result;
}

Tensor NanoQuantFactors::forward(const Tensor& input) const {
    samsung_internal::validate_matrix(input, "NanoQuantFactors::forward");
    if (input.shape()[1] != v_sign.cols)
        throw std::invalid_argument("NanoQuantFactors::forward: input feature mismatch");
    return input.matmul(reconstruct().transpose());
}

NanoQuantFactors nano_quantize(const Tensor& weights, const NanoQuantConfig& config,
                               const Tensor& input_importance,
                               const Tensor& output_importance) {
    samsung_internal::validate_matrix(weights, "nano_quantize");
    const size_t out_features = weights.shape()[0];
    const size_t in_features = weights.shape()[1];
    if (config.rank == 0 || config.rank > std::min(out_features, in_features))
        throw std::invalid_argument("nano_quantize: invalid rank");
    if (config.outer_iterations == 0 || config.projection_iterations == 0)
        throw std::invalid_argument("nano_quantize: iteration counts must be > 0");
    if (!(config.regularization > 0.0) || !(config.epsilon > 0.0))
        throw std::invalid_argument("nano_quantize: regularization and epsilon must be > 0");
    validate_importance(input_importance, in_features, "input_importance");
    validate_importance(output_importance, out_features, "output_importance");

    // The reference implementation transposes wide layers, then swaps factors
    // and scales back. This preserves the better-conditioned ADMM orientation.
    if (out_features < in_features) {
        NanoQuantFactors transposed = nano_quantize(
            weights.transpose(), config, output_importance, input_importance);
        const Tensor original_u = unpack_binary_signs(transposed.v_sign).transpose();
        const Tensor original_v = unpack_binary_signs(transposed.u_sign).transpose();
        return {pack_binary_signs(original_u), pack_binary_signs(original_v),
                transposed.post_scale, transposed.pre_scale};
    }

    std::vector<double> norm_in(in_features, 1.0), norm_out(out_features, 1.0);
    if (input_importance.size())
        for (size_t i = 0; i < in_features; ++i)
            norm_in[i] = std::max(std::sqrt(input_importance.data()[i]), config.epsilon);
    if (output_importance.size())
        for (size_t i = 0; i < out_features; ++i)
            norm_out[i] = std::max(std::sqrt(output_importance.data()[i]), config.epsilon);

    Tensor normalized(weights.shape(), 0.0);
    for (size_t out = 0; out < out_features; ++out)
        for (size_t in = 0; in < in_features; ++in)
            at(normalized, out, in) = at(weights, out, in) * norm_out[out] * norm_in[in];

    Tensor a_ls({out_features, config.rank}, 0.0);
    Tensor b_ls({config.rank, in_features}, 0.0);
    std::mt19937 generator(config.seed);
    std::normal_distribution<double> normal(0.0, 1.0);
    for (double& value : a_ls.data()) value = normal(generator);
    for (double& value : b_ls.data()) value = normal(generator);
    Tensor a_z = samsung_internal::rank_one_magnitude(a_ls, config.projection_iterations);
    Tensor b_z = samsung_internal::rank_one_magnitude(b_ls, config.projection_iterations);
    Tensor a_dual = a_ls - a_z;
    Tensor b_dual = b_ls - b_z;

    for (size_t iteration = 0; iteration < config.outer_iterations; ++iteration) {
        const double progress = static_cast<double>(iteration) /
                                static_cast<double>(config.outer_iterations);
        const double rho = progress * progress * progress;

        Tensor x_a({in_features, config.rank}, 0.0);
        for (size_t k = 0; k < config.rank; ++k) {
            double norm = 0.0;
            for (size_t in = 0; in < in_features; ++in) norm += at(b_z, k, in) * at(b_z, k, in);
            norm = std::max(std::sqrt(norm), config.epsilon);
            for (size_t in = 0; in < in_features; ++in) at(x_a, in, k) = at(b_z, k, in) / norm;
        }
        a_ls = admm_least_squares(x_a, normalized.transpose(), a_z.transpose(),
                                  a_dual.transpose(), rho, config.regularization).transpose();

        Tensor x_b({out_features, config.rank}, 0.0);
        for (size_t k = 0; k < config.rank; ++k) {
            double norm = 0.0;
            for (size_t out = 0; out < out_features; ++out) norm += at(a_z, out, k) * at(a_z, out, k);
            norm = std::max(std::sqrt(norm), config.epsilon);
            for (size_t out = 0; out < out_features; ++out) at(x_b, out, k) = at(a_z, out, k) / norm;
        }
        b_ls = admm_least_squares(x_b, normalized, b_z, b_dual,
                                  rho, config.regularization);

        a_z = samsung_internal::rank_one_magnitude(a_ls + a_dual, config.projection_iterations);
        b_z = samsung_internal::rank_one_magnitude(b_ls + b_dual, config.projection_iterations);
        a_dual += a_ls - a_z;
        b_dual += b_ls - b_z;
    }

    Tensor a({out_features, config.rank}, 0.0);
    Tensor b({config.rank, in_features}, 0.0);
    for (size_t out = 0; out < out_features; ++out)
        for (size_t k = 0; k < config.rank; ++k) at(a, out, k) = at(a_z, out, k) / norm_out[out];
    for (size_t k = 0; k < config.rank; ++k)
        for (size_t in = 0; in < in_features; ++in) at(b, k, in) = at(b_z, k, in) / norm_in[in];

    const double norm_a = std::max(frobenius_norm(a), config.epsilon);
    const double norm_b = std::max(frobenius_norm(b), config.epsilon);
    const double balance = std::sqrt(norm_b / norm_a);
    for (double& value : a.data()) value *= balance;
    for (double& value : b.data()) value /= balance;

    // As in the official export path, absorb per-latent normalization into A,
    // then extract only pre/post mean-magnitude scales.
    for (size_t k = 0; k < config.rank; ++k) {
        double norm = 0.0;
        for (size_t out = 0; out < out_features; ++out) norm += at(a_z, out, k) * at(a_z, out, k);
        norm = std::max(std::sqrt(norm), config.epsilon);
        for (size_t out = 0; out < out_features; ++out) at(a, out, k) /= norm;
    }

    Tensor pre({in_features}, 0.0), post({out_features}, 0.0);
    for (size_t in = 0; in < in_features; ++in) {
        for (size_t k = 0; k < config.rank; ++k) pre.data()[in] += std::abs(at(b, k, in));
        pre.data()[in] /= static_cast<double>(config.rank);
    }
    for (size_t out = 0; out < out_features; ++out) {
        for (size_t k = 0; k < config.rank; ++k) post.data()[out] += std::abs(at(a, out, k));
        post.data()[out] /= static_cast<double>(config.rank);
    }
    return {pack_binary_signs(a), pack_binary_signs(b), pre, post};
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
