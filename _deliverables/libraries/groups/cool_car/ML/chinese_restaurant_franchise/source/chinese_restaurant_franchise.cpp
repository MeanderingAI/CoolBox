#include "chinese_restaurant_franchise.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace ml {

ChineseRestaurantFranchise::ChineseRestaurantFranchise(
    double concentration,
    double dish_concentration)
    : ChineseRestaurantFranchise(
          concentration, dish_concentration, std::random_device{}()) {}

ChineseRestaurantFranchise::ChineseRestaurantFranchise(
    double concentration,
    double dish_concentration,
    std::uint32_t seed)
    : concentration_(concentration),
      dish_concentration_(dish_concentration),
      generator_(seed) {
    if (!std::isfinite(concentration_) || concentration_ <= 0.0) {
        throw std::invalid_argument(
            "ChineseRestaurantFranchise: concentration must be finite and positive");
    }
    if (!std::isfinite(dish_concentration_) || dish_concentration_ <= 0.0) {
        throw std::invalid_argument(
            "ChineseRestaurantFranchise: dish concentration must be finite and positive");
    }
}

std::size_t ChineseRestaurantFranchise::add_restaurant() {
    if (restaurants_.size() == restaurants_.max_size()) {
        throw std::length_error("ChineseRestaurantFranchise: too many restaurants");
    }
    restaurants_.emplace_back();
    return restaurants_.size() - 1;
}

FranchiseAssignment ChineseRestaurantFranchise::add_customer(std::size_t restaurant) {
    if (restaurant >= restaurants_.size()) {
        throw std::out_of_range("ChineseRestaurantFranchise: restaurant index is out of range");
    }

    Restaurant& current = restaurants_[restaurant];
    if (current.customers == std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("ChineseRestaurantFranchise: customer count overflow");
    }

    std::vector<double> seating_weights;
    seating_weights.reserve(current.tables.size() + 1);
    for (const FranchiseTable& table : current.tables) {
        seating_weights.push_back(static_cast<double>(table.customers));
    }
    seating_weights.push_back(concentration_);
    const std::size_t seating_choice =
        std::discrete_distribution<std::size_t>(
            seating_weights.begin(), seating_weights.end())(generator_);

    if (seating_choice < current.tables.size()) {
        FranchiseTable& table = current.tables[seating_choice];
        ++table.customers;
        ++current.customers;
        return {seating_choice, table.dish, false, false};
    }

    if (num_tables_ == std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error("ChineseRestaurantFranchise: table count overflow");
    }

    std::vector<double> dish_weights;
    dish_weights.reserve(dish_table_counts_.size() + 1);
    for (std::size_t count : dish_table_counts_) {
        dish_weights.push_back(static_cast<double>(count));
    }
    dish_weights.push_back(dish_concentration_);
    const std::size_t dish_choice =
        std::discrete_distribution<std::size_t>(
            dish_weights.begin(), dish_weights.end())(generator_);
    const bool new_dish = dish_choice == dish_table_counts_.size();

    if (new_dish && dish_table_counts_.size() == dish_table_counts_.max_size()) {
        throw std::length_error("ChineseRestaurantFranchise: too many dishes");
    }
    current.tables.reserve(current.tables.size() + 1);
    if (new_dish) {
        dish_table_counts_.reserve(dish_table_counts_.size() + 1);
        dish_table_counts_.push_back(1);
    } else {
        ++dish_table_counts_[dish_choice];
    }

    current.tables.push_back({1, dish_choice});
    ++current.customers;
    ++num_tables_;
    return {seating_choice, dish_choice, true, new_dish};
}

std::vector<FranchiseAssignment> ChineseRestaurantFranchise::add_customers(
    std::size_t restaurant,
    std::size_t count) {
    std::vector<FranchiseAssignment> assignments;
    assignments.reserve(count);
    for (std::size_t customer = 0; customer < count; ++customer) {
        assignments.push_back(add_customer(restaurant));
    }
    return assignments;
}

std::size_t ChineseRestaurantFranchise::customers_in_restaurant(
    std::size_t restaurant) const {
    if (restaurant >= restaurants_.size()) {
        throw std::out_of_range("ChineseRestaurantFranchise: restaurant index is out of range");
    }
    return restaurants_[restaurant].customers;
}

const std::vector<FranchiseTable>& ChineseRestaurantFranchise::tables_for_restaurant(
    std::size_t restaurant) const {
    if (restaurant >= restaurants_.size()) {
        throw std::out_of_range("ChineseRestaurantFranchise: restaurant index is out of range");
    }
    return restaurants_[restaurant].tables;
}

} // namespace ml
