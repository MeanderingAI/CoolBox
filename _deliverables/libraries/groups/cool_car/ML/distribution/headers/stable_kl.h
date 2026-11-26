#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace mytrix {
class DenseMatrix;
class DenseVector;
}

namespace ml {
namespace stable_kl {

std::vector<double> expm1_minus_x(const std::vector<double>& values);

std::vector<double> normal_kl(
    const std::vector<double>& mu,
    const std::vector<double>& logvar,
    const std::vector<double>& mu_target,
    const std::vector<double>& logvar_target);

std::vector<double> kl_from_logits(
    const std::vector<double>& teacher_logits,
    const std::vector<double>& student_logits,
    std::size_t rows,
    std::size_t classes);

enum class Reduction {
    None,
    BatchMean,
    Mean,
    Sum
};

std::vector<double> kl_divergence(
    const std::vector<double>& target,
    const std::vector<double>& input,
    bool log_target,
    Reduction reduction,
    std::size_t batch_size,
    std::size_t batch_columns);

Reduction parse_reduction(const std::string& reduction);

mytrix::DenseMatrix expm1_minus_x(const mytrix::DenseMatrix& values);

mytrix::DenseMatrix normal_kl(
    const mytrix::DenseMatrix& mu,
    const mytrix::DenseMatrix& logvar,
    const mytrix::DenseMatrix& mu_target,
    const mytrix::DenseMatrix& logvar_target);

mytrix::DenseVector kl_from_logits(
    const mytrix::DenseMatrix& teacher_logits,
    const mytrix::DenseMatrix& student_logits);

mytrix::DenseMatrix kl_divergence(
    const mytrix::DenseMatrix& target,
    const mytrix::DenseMatrix& input,
    bool log_target);

} // namespace stable_kl
} // namespace ml
