#include "tyst_framework.hpp"

#include "indian_buffet_process.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

using ml::IndianBuffetProcess;

TYST_TEST(IndianBuffetProcessTest, RejectsInvalidConcentrations) {
    TYST_EXPECT_THROW(IndianBuffetProcess(0.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(IndianBuffetProcess(-1.0, 1), std::invalid_argument);
    TYST_EXPECT_THROW(IndianBuffetProcess(
        std::numeric_limits<double>::infinity(), 1), std::invalid_argument);
    TYST_EXPECT_THROW(IndianBuffetProcess(
        std::numeric_limits<double>::quiet_NaN(), 1), std::invalid_argument);
}

TYST_TEST(IndianBuffetProcessTest, TracksFeatureAssignmentsAndIsReproducible) {
    IndianBuffetProcess first(1.5, 42);
    IndianBuffetProcess second(1.5, 42);

    const auto first_assignments = first.add_customers(100);
    const auto second_assignments = second.add_customers(100);

    TYST_EXPECT_EQ(first_assignments, second_assignments);
    TYST_EXPECT_EQ(first.customer_features(), second.customer_features());
    TYST_EXPECT_EQ(first.num_customers(), 100u);
    TYST_EXPECT_EQ(first.num_features(), first.feature_counts().size());

    std::vector<std::size_t> observed_counts(first.num_features(), 0);
    for (std::size_t customer = 0; customer < first_assignments.size(); ++customer) {
        TYST_EXPECT_EQ(first_assignments[customer], first.customer_features()[customer]);
        for (std::size_t feature : first_assignments[customer]) {
            TYST_EXPECT_LT(feature, first.num_features());
            ++observed_counts[feature];
        }
    }
    TYST_EXPECT_EQ(observed_counts, first.feature_counts());
    for (std::size_t count : first.feature_counts()) {
        TYST_EXPECT_GT(count, 0u);
    }
}

TYST_TEST(IndianBuffetProcessTest, FirstCustomerReceivesPoissonDistributedFeatures) {
    constexpr std::size_t trials = 3000;
    constexpr double concentration = 2.0;
    std::size_t total_features = 0;

    for (std::size_t seed = 0; seed < trials; ++seed) {
        IndianBuffetProcess process(concentration, static_cast<std::uint32_t>(seed));
        total_features += process.add_customer().size();
    }

    const double observed_mean =
        static_cast<double>(total_features) / static_cast<double>(trials);
    TYST_EXPECT_NEAR(observed_mean, concentration, 0.15);
}
