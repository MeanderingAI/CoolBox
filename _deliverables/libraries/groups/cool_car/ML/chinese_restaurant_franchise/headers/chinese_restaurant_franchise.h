#ifndef CHINESE_RESTAURANT_FRANCHISE_H
#define CHINESE_RESTAURANT_FRANCHISE_H

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace ml {

/// A table's current size and the globally shared dish it serves.
struct FranchiseTable {
    std::size_t customers = 0;
    std::size_t dish = 0;
};

/// The seating result for one customer.
struct FranchiseAssignment {
    std::size_t table = 0;
    std::size_t dish = 0;
    bool new_table = false;
    bool new_dish = false;
};

/// Samples customer seating and shared dishes from a Chinese Restaurant
/// Franchise, the hierarchical Dirichlet process' restaurant representation.
///
/// Customers join tables within their restaurant proportional to table size,
/// or open a table with mass @p concentration. Each new table chooses an
/// existing global dish proportional to the number of tables serving it, or
/// introduces a dish with mass @p dish_concentration.
class ChineseRestaurantFranchise {
public:
    /// Construct with nondeterministic random seed.
    ChineseRestaurantFranchise(double concentration, double dish_concentration);

    /// Construct with a seed for reproducible sampling.
    ChineseRestaurantFranchise(
        double concentration,
        double dish_concentration,
        std::uint32_t seed);

    /// Add an empty restaurant and return its zero-based index.
    std::size_t add_restaurant();

    /// Seat a customer in an existing restaurant and return its table and dish.
    FranchiseAssignment add_customer(std::size_t restaurant);

    /// Seat multiple customers in one restaurant, returning assignments in order.
    std::vector<FranchiseAssignment> add_customers(
        std::size_t restaurant,
        std::size_t count);

    double concentration() const { return concentration_; }
    double dish_concentration() const { return dish_concentration_; }
    std::size_t num_restaurants() const { return restaurants_.size(); }
    std::size_t num_tables() const { return num_tables_; }
    std::size_t num_dishes() const { return dish_table_counts_.size(); }
    std::size_t customers_in_restaurant(std::size_t restaurant) const;
    const std::vector<FranchiseTable>& tables_for_restaurant(
        std::size_t restaurant) const;
    const std::vector<std::size_t>& dish_table_counts() const {
        return dish_table_counts_;
    }

private:
    struct Restaurant {
        std::size_t customers = 0;
        std::vector<FranchiseTable> tables;
    };

    double concentration_;
    double dish_concentration_;
    std::size_t num_tables_ = 0;
    std::vector<Restaurant> restaurants_;
    std::vector<std::size_t> dish_table_counts_;
    std::mt19937 generator_;
};

} // namespace ml

#endif // CHINESE_RESTAURANT_FRANCHISE_H
