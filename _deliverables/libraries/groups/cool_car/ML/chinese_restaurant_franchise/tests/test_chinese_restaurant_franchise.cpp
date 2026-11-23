#include "tyst_framework.hpp"

#include "chinese_restaurant_franchise.h"
#include "hierarchical_dirichlet_process.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

using ml::ChineseRestaurantFranchise;
using ml::HierarchicalDirichletProcess;

TYST_TEST(ChineseRestaurantFranchiseTest, RejectsInvalidConcentrationsAndRestaurants) {
    TYST_EXPECT_THROW(ChineseRestaurantFranchise(0.0, 1.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(ChineseRestaurantFranchise(1.0, -1.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(ChineseRestaurantFranchise(
        std::numeric_limits<double>::infinity(), 1.0, 1), std::invalid_argument);

    ChineseRestaurantFranchise franchise(1.0, 1.0, 1);
    TYST_EXPECT_THROW(franchise.add_customer(0), std::out_of_range);
}

TYST_TEST(ChineseRestaurantFranchiseTest, TracksSharedDishesAndIsReproducible) {
    ChineseRestaurantFranchise first(1.2, 0.8, 24);
    ChineseRestaurantFranchise second(1.2, 0.8, 24);
    const std::size_t first_restaurant = first.add_restaurant();
    const std::size_t second_restaurant = first.add_restaurant();
    TYST_EXPECT_EQ(first_restaurant, 0u);
    TYST_EXPECT_EQ(second_restaurant, 1u);

    second.add_restaurant();
    second.add_restaurant();
    for (std::size_t customer = 0; customer < 150; ++customer) {
        const std::size_t restaurant = customer % 2;
        const auto first_assignment = first.add_customer(restaurant);
        const auto second_assignment = second.add_customer(restaurant);
        TYST_EXPECT_EQ(first_assignment.table, second_assignment.table);
        TYST_EXPECT_EQ(first_assignment.dish, second_assignment.dish);
        TYST_EXPECT_EQ(first_assignment.new_table, second_assignment.new_table);
        TYST_EXPECT_EQ(first_assignment.new_dish, second_assignment.new_dish);
    }

    std::vector<std::size_t> observed_dish_table_counts(first.num_dishes(), 0);
    std::size_t observed_tables = 0;
    for (std::size_t restaurant = 0; restaurant < first.num_restaurants(); ++restaurant) {
        std::size_t observed_customers = 0;
        const auto& tables = first.tables_for_restaurant(restaurant);
        observed_tables += tables.size();
        for (const auto& table : tables) {
            TYST_EXPECT_GT(table.customers, 0u);
            TYST_EXPECT_LT(table.dish, first.num_dishes());
            observed_customers += table.customers;
            ++observed_dish_table_counts[table.dish];
        }
        TYST_EXPECT_EQ(observed_customers, first.customers_in_restaurant(restaurant));
    }
    TYST_EXPECT_EQ(observed_tables, first.num_tables());
    TYST_EXPECT_EQ(observed_dish_table_counts, first.dish_table_counts());
}

TYST_TEST(ChineseRestaurantFranchiseTest, NewTableChoosesGlobalDishByTablePopularity) {
    constexpr std::size_t trials = 5000;
    std::size_t new_dish_assignments = 0;

    for (std::size_t seed = 0; seed < trials; ++seed) {
        ChineseRestaurantFranchise franchise(1.0, 1.0, static_cast<std::uint32_t>(seed));
        const std::size_t restaurant = franchise.add_restaurant();
        franchise.add_customer(restaurant);
        const auto second = franchise.add_customer(restaurant);
        if (second.new_dish) {
            ++new_dish_assignments;
        }
    }

    const double observed_probability =
        static_cast<double>(new_dish_assignments) / static_cast<double>(trials);
    TYST_EXPECT_NEAR(observed_probability, 0.25, 0.04);
}

TYST_TEST(HierarchicalDirichletProcessTest, SamplesCollapsedFranchiseAllocations) {
    HierarchicalDirichletProcess hdp(1.0, 1.0, 42);
    const std::size_t first_restaurant = hdp.add_restaurant();
    const std::size_t second_restaurant = hdp.add_restaurant();

    const auto first = hdp.add_customer(first_restaurant);
    const auto second = hdp.add_customer(second_restaurant);

    TYST_EXPECT_TRUE(first.new_table);
    TYST_EXPECT_TRUE(first.new_dish);
    TYST_EXPECT_TRUE(second.new_table);
    TYST_EXPECT_LT(first.dish, hdp.num_dishes());
    TYST_EXPECT_LT(second.dish, hdp.num_dishes());
    TYST_EXPECT_EQ(hdp.tables_for_restaurant(first_restaurant)[first.table].dish, first.dish);
    TYST_EXPECT_EQ(hdp.tables_for_restaurant(second_restaurant)[second.table].dish, second.dish);
}
