#include "chinese_restaurant_process.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace ml {

ChineseRestaurantProcess::ChineseRestaurantProcess(double concentration)
    : ChineseRestaurantProcess(concentration, std::random_device{}()) {}

ChineseRestaurantProcess::ChineseRestaurantProcess(double concentration, std::uint32_t seed)
    : concentration_(concentration), generator_(seed) {
    if (!std::isfinite(concentration_) || concentration_ <= 0.0) {
        throw std::invalid_argument(
            "ChineseRestaurantProcess: concentration must be finite and positive");
    }
}

std::size_t ChineseRestaurantProcess::seat_customer() {
    if (customers_seated_ == std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("ChineseRestaurantProcess: customer count overflow");
    }

    std::vector<double> weights;
    weights.reserve(table_counts_.size() + 1);
    for (std::size_t count : table_counts_) {
        weights.push_back(static_cast<double>(count));
    }
    weights.push_back(concentration_);

    std::discrete_distribution<std::size_t> distribution(weights.begin(), weights.end());
    const std::size_t choice = distribution(generator_);
    if (choice == table_counts_.size()) {
        table_counts_.push_back(1);
    } else {
        ++table_counts_[choice];
    }
    ++customers_seated_;
    return choice;
}

std::vector<std::size_t> ChineseRestaurantProcess::seat_customers(std::size_t count) {
    std::vector<std::size_t> assignments;
    assignments.reserve(count);
    for (std::size_t customer = 0; customer < count; ++customer) {
        assignments.push_back(seat_customer());
    }
    return assignments;
}

} // namespace ml
