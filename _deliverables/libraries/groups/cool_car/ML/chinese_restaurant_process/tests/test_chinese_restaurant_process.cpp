#include "tyst_framework.hpp"

#include "chinese_restaurant_process.h"

#include <cmath>
#include <limits>
#include <stdexcept>

using ml::ChineseRestaurantProcess;

TYST_TEST(ChineseRestaurantProcessTest, RejectsInvalidConcentrations) {
    TYST_EXPECT_THROW(ChineseRestaurantProcess(0.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(ChineseRestaurantProcess(-1.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(ChineseRestaurantProcess(
        std::numeric_limits<double>::infinity(), 1), std::invalid_argument);
    TYST_EXPECT_THROW(ChineseRestaurantProcess(
        std::numeric_limits<double>::quiet_NaN(), 1), std::invalid_argument);
}

TYST_TEST(ChineseRestaurantProcessTest, SamplesConsistentAssignmentsAndIsReproducible) {
    ChineseRestaurantProcess first(1.5, 42);
    ChineseRestaurantProcess second(1.5, 42);

    const auto first_assignments = first.seat_customers(1000);
    const auto second_assignments = second.seat_customers(1000);

    TYST_EXPECT_EQ(first_assignments, second_assignments);
    TYST_EXPECT_EQ(first.customers_seated(), 1000u);
    TYST_EXPECT_EQ(first.table_counts().size(), first.num_tables());

    std::size_t total = 0;
    for (std::size_t count : first.table_counts()) {
        TYST_EXPECT_GT(count, 0u);
        total += count;
    }
    TYST_EXPECT_EQ(total, first.customers_seated());
    TYST_EXPECT_EQ(first_assignments.front(), 0u);
    for (std::size_t table : first_assignments) {
        TYST_EXPECT_LT(table, first.num_tables());
    }
}

TYST_TEST(ChineseRestaurantProcessTest, NewTableProbabilityMatchesConcentration) {
    constexpr std::size_t trials = 5000;
    std::size_t new_table_count = 0;

    for (std::size_t seed = 0; seed < trials; ++seed) {
        ChineseRestaurantProcess process(1.0, static_cast<std::uint32_t>(seed));
        process.seat_customer();
        process.seat_customer();
        if (process.num_tables() == 2) {
            ++new_table_count;
        }
    }

    const double observed_probability =
        static_cast<double>(new_table_count) / static_cast<double>(trials);
    TYST_EXPECT_NEAR(observed_probability, 0.5, 0.04);
}
