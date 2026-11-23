#ifndef CHINESE_RESTAURANT_PROCESS_H
#define CHINESE_RESTAURANT_PROCESS_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace ml {

/// Samples customer-to-table assignments from a Chinese Restaurant Process.
///
/// Given concentration alpha, the next customer joins an existing table with
/// probability proportional to its occupancy, or starts a new table with
/// probability alpha / (customers_seated + alpha).
class ChineseRestaurantProcess {
public:
    /// Construct with a nondeterministic seed.
    explicit ChineseRestaurantProcess(double concentration);

    /// Construct with a seed for reproducible sampling.
    ChineseRestaurantProcess(double concentration, std::uint32_t seed);

    /// Seat one customer and return the zero-based table index.
    std::size_t seat_customer();

    /// Seat @p count customers and return their table indices in order.
    std::vector<std::size_t> seat_customers(std::size_t count);

    double concentration() const { return concentration_; }
    std::size_t customers_seated() const { return customers_seated_; }
    std::size_t num_tables() const { return table_counts_.size(); }
    const std::vector<std::size_t>& table_counts() const { return table_counts_; }

private:
    double concentration_;
    std::size_t customers_seated_ = 0;
    std::vector<std::size_t> table_counts_;
    std::mt19937 generator_;
};

} // namespace ml

#endif // CHINESE_RESTAURANT_PROCESS_H
