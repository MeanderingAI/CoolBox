#include <tyst_framework.hpp>
#include "scheduling.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <vector>

using namespace trekker::algorithm::scheduling;

TEST(SchedulingRoundRobinTest, CyclesThroughJobs) {
    auto sequence = round_robin_sequence(3, 8);
    EXPECT_EQ(sequence, (std::vector<job_id_t>{0, 1, 2, 0, 1, 2, 0, 1}));
}

TEST(SchedulingStrideTest, FavorsHigherTicketCounts) {
    auto sequence = stride_sequence({1, 3}, 8);
    std::size_t count_first = std::count(sequence.begin(), sequence.end(), 0);
    std::size_t count_second = std::count(sequence.begin(), sequence.end(), 1);
    EXPECT_LT(count_first, count_second);
}

TEST(SchedulingLotteryTest, DeterministicSeedProducesStableSequence) {
    auto first = lottery_sequence({1, 2, 3}, 6, 1234);
    auto second = lottery_sequence({1, 2, 3}, 6, 1234);
    EXPECT_EQ(first, second);
}

TEST(SchedulingPriorityTest, OrdersHigherPriorityFirst) {
    auto order = priority_order({2, 7, 5, 7});
    EXPECT_EQ(order, (std::vector<job_id_t>{1, 3, 2, 0}));
}

TEST(SchedulingDeadlineTest, OrdersEarliestDeadlineFirst) {
    auto order = earliest_deadline_first_order({9, 2, 5, 2});
    EXPECT_EQ(order, (std::vector<job_id_t>{1, 3, 2, 0}));
}

TEST(SchedulingShortestJobFirstTest, OrdersSmallestBurstFirst) {
    auto order = shortest_job_first_order({9, 2, 5, 2});
    EXPECT_EQ(order, (std::vector<job_id_t>{1, 3, 2, 0}));
}

TEST(SchedulingShortestRemainingTimeTest, EmitsPreemptiveExecutionTrace) {
    auto sequence = shortest_remaining_time_sequence({3, 1, 2});
    EXPECT_EQ(sequence, (std::vector<job_id_t>{1, 2, 2, 0, 0, 0}));
}

TEST(SchedulingEdgeCasesTest, HandlesEmptyInputs) {
    EXPECT_TRUE(round_robin_sequence(0, 10).empty());
    EXPECT_TRUE(stride_sequence({}, 10).empty());
    EXPECT_TRUE(lottery_sequence({}, 10).empty());
    EXPECT_TRUE(priority_order({}).empty());
    EXPECT_TRUE(earliest_deadline_first_order({}).empty());
    EXPECT_TRUE(shortest_job_first_order({}).empty());
    EXPECT_TRUE(shortest_remaining_time_sequence({}).empty());
}
