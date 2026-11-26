#include "stable_kl.h"
#include "matrix_dense.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace stable_kl {

double expm1_minus_x_scalar(double x) {
    if (std::abs(x) < 1e-5) {
        return x * x * (
            0.5 +
            x * (1.0 / 6.0 +
                x * (1.0 / 24.0 +
                    x * (1.0 / 120.0 + x / 720.0))));
    }
    return std::expm1(x) - x;
}

std::vector<double> expm1_minus_x(const std::vector<double>& values) {
    std::vector<double> result(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        result[i] = expm1_minus_x_scalar(values[i]);
    }
    return result;
}

std::vector<double> normal_kl(
    const std::vector<double>& mu,
    const std::vector<double>& logvar,
    const std::vector<double>& mu_target,
    const std::vector<double>& logvar_target) {
    const std::size_t size = mu.size();
    if (logvar.size() != size || mu_target.size() != size || logvar_target.size() != size) {
        throw std::invalid_argument("normal_kl inputs must have the same length");
    }

    std::vector<double> result(size);
    for (std::size_t i = 0; i < size; ++i) {
        const double delta = mu[i] - mu_target[i];
        const double target_scale = std::exp(0.5 * logvar_target[i]);
        const double scaled_mean = delta / target_scale;
        const double log_ratio = logvar[i] - logvar_target[i];
        result[i] = 0.5 * (
            scaled_mean * scaled_mean +
            expm1_minus_x_scalar(log_ratio));
    }
    return result;
}

std::vector<double> kl_from_logits(
    const std::vector<double>& teacher_logits,
    const std::vector<double>& student_logits,
    std::size_t rows,
    std::size_t classes) {
    if (classes == 0) {
        throw std::invalid_argument("classes must be positive");
    }
    if (teacher_logits.size() != rows * classes ||
        student_logits.size() != rows * classes) {
        throw std::invalid_argument("logit arrays must contain rows * classes values");
    }

    std::vector<double> result(rows, 0.0);
    for (std::size_t row = 0; row < rows; ++row) {
        const std::size_t offset = row * classes;

        double teacher_max = teacher_logits[offset];
        double student_max = student_logits[offset];
        for (std::size_t column = 1; column < classes; ++column) {
            teacher_max = std::max(teacher_max, teacher_logits[offset + column]);
            student_max = std::max(student_max, student_logits[offset + column]);
        }

        double q_sum = 0.0;
        std::vector<double> q(classes);
        for (std::size_t column = 0; column < classes; ++column) {
            q[column] = std::exp(student_logits[offset + column] - student_max);
            q_sum += q[column];
        }

        double correction = 0.0;
        for (std::size_t column = 0; column < classes; ++column) {
            q[column] /= q_sum;
            const double delta =
                (teacher_logits[offset + column] - teacher_max) -
                (student_logits[offset + column] - student_max);
            correction += q[column] * std::expm1(delta);
        }
        result[row] = std::log1p(correction);
    }
    return result;
}

Reduction parse_reduction(const std::string& reduction) {
    if (reduction == "none") return Reduction::None;
    if (reduction == "batchmean") return Reduction::BatchMean;
    if (reduction == "mean") return Reduction::Mean;
    if (reduction == "sum") return Reduction::Sum;
    throw std::invalid_argument("reduction must be 'none', 'batchmean', 'mean', or 'sum'");
}

std::vector<double> kl_divergence(
    const std::vector<double>& target,
    const std::vector<double>& input,
    bool log_target,
    Reduction reduction,
    std::size_t batch_size,
    std::size_t batch_columns) {
    if (target.size() != input.size()) {
        throw std::invalid_argument("target and input must have the same length");
    }

    std::vector<double> elements(target.size());
    double sum = 0.0;
    for (std::size_t i = 0; i < target.size(); ++i) {
        const double probability = log_target ? std::exp(target[i]) : target[i];
        const double log_probability = log_target ? target[i] : std::log(target[i]);
        elements[i] = probability * (log_probability - input[i]);
        sum += elements[i];
    }

    if (reduction == Reduction::None) {
        return elements;
    }
    if (reduction == Reduction::Sum) {
        return {sum};
    }
    if (reduction == Reduction::Mean) {
        if (target.empty()) return {0.0};
        return {sum / static_cast<double>(target.size())};
    }
    if (reduction == Reduction::BatchMean) {
        if (batch_size == 0) return {0.0};
        if (batch_columns != 0 && target.size() != batch_size * batch_columns) {
            throw std::invalid_argument("batch dimensions do not match target length");
        }
        return {sum / static_cast<double>(batch_size)};
    }
    return elements;
}

mytrix::DenseMatrix expm1_minus_x(const mytrix::DenseMatrix& values) {
    mytrix::DenseMatrix result(values.rows(), values.cols());
    for (int i = 0; i < values.rows(); ++i) {
        for (int j = 0; j < values.cols(); ++j) {
            result(i, j) = expm1_minus_x_scalar(values(i, j));
        }
    }
    return result;
}

mytrix::DenseMatrix normal_kl(
    const mytrix::DenseMatrix& mu,
    const mytrix::DenseMatrix& logvar,
    const mytrix::DenseMatrix& mu_target,
    const mytrix::DenseMatrix& logvar_target) {
    if (mu.rows() != logvar.rows() || mu.cols() != logvar.cols() ||
        mu.rows() != mu_target.rows() || mu.cols() != mu_target.cols() ||
        mu.rows() != logvar_target.rows() || mu.cols() != logvar_target.cols()) {
        throw std::invalid_argument("normal_kl matrices must have identical shapes");
    }

    mytrix::DenseMatrix result(mu.rows(), mu.cols());
    for (int i = 0; i < mu.rows(); ++i) {
        for (int j = 0; j < mu.cols(); ++j) {
            const double delta = mu(i, j) - mu_target(i, j);
            const double target_scale = std::exp(0.5 * logvar_target(i, j));
            const double scaled_mean = delta / target_scale;
            const double log_ratio = logvar(i, j) - logvar_target(i, j);
            result(i, j) = 0.5 * (
                scaled_mean * scaled_mean +
                expm1_minus_x_scalar(log_ratio));
        }
    }
    return result;
}

mytrix::DenseVector kl_from_logits(
    const mytrix::DenseMatrix& teacher_logits,
    const mytrix::DenseMatrix& student_logits) {
    if (teacher_logits.rows() != student_logits.rows() ||
        teacher_logits.cols() != student_logits.cols()) {
        throw std::invalid_argument("logit matrices must have identical shapes");
    }
    if (teacher_logits.cols() == 0) {
        throw std::invalid_argument("logit matrices must contain at least one class");
    }

    std::vector<double> teacher(
        teacher_logits.rows() * teacher_logits.cols());
    std::vector<double> student(
        student_logits.rows() * student_logits.cols());
    for (int i = 0; i < teacher_logits.rows(); ++i) {
        for (int j = 0; j < teacher_logits.cols(); ++j) {
            const std::size_t index =
                static_cast<std::size_t>(i * teacher_logits.cols() + j);
            teacher[index] = teacher_logits(i, j);
            student[index] = student_logits(i, j);
        }
    }

    const auto values = kl_from_logits(
        teacher, student,
        static_cast<std::size_t>(teacher_logits.rows()),
        static_cast<std::size_t>(teacher_logits.cols()));
    return mytrix::DenseVector(
        values,
        static_cast<std::size_t>(values.size()));
}

mytrix::DenseMatrix kl_divergence(
    const mytrix::DenseMatrix& target,
    const mytrix::DenseMatrix& input,
    bool log_target) {
    if (target.rows() != input.rows() || target.cols() != input.cols()) {
        throw std::invalid_argument("target and input must have identical shapes");
    }

    mytrix::DenseMatrix result(target.rows(), target.cols());
    for (int i = 0; i < target.rows(); ++i) {
        for (int j = 0; j < target.cols(); ++j) {
            const double probability =
                log_target ? std::exp(target(i, j)) : target(i, j);
            const double log_probability =
                log_target ? target(i, j) : std::log(target(i, j));
            result(i, j) = probability * (log_probability - input(i, j));
        }
    }
    return result;
}

} // namespace stable_kl
} // namespace ml
