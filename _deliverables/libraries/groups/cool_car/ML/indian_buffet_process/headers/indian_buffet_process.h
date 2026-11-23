#ifndef INDIAN_BUFFET_PROCESS_H
#define INDIAN_BUFFET_PROCESS_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace ml {

/// Samples binary feature assignments from the Indian Buffet Process.
///
/// For customer i (one-based), each existing feature k is selected with
/// probability m_k / (i - 1 + alpha), where m_k is its current customer count.
/// The customer also receives Poisson(alpha / i) new features.
class IndianBuffetProcess {
public:
    /// Construct with a nondeterministic seed.
    explicit IndianBuffetProcess(double concentration);

    /// Construct with a seed for reproducible sampling.
    IndianBuffetProcess(double concentration, std::uint32_t seed);

    /// Add one customer and return the zero-based indices of its features.
    std::vector<std::size_t> add_customer();

    /// Add @p count customers and return their feature-index lists in order.
    std::vector<std::vector<std::size_t>> add_customers(std::size_t count);

    double concentration() const { return concentration_; }
    std::size_t num_customers() const { return customer_features_.size(); }
    std::size_t num_features() const { return feature_counts_.size(); }
    const std::vector<std::vector<std::size_t>>& customer_features() const {
        return customer_features_;
    }
    const std::vector<std::size_t>& feature_counts() const { return feature_counts_; }

private:
    double concentration_;
    std::vector<std::vector<std::size_t>> customer_features_;
    std::vector<std::size_t> feature_counts_;
    std::mt19937 generator_;
};

} // namespace ml

#endif // INDIAN_BUFFET_PROCESS_H
