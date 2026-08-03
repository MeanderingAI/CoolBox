#include "tyst_framework.hpp"

#include "endgame_table_compression.h"

#include <vector>

using namespace ml::endgame;

namespace {

std::vector<EndgameEntry> make_small_wcdbl_table() {
    return {
        {{0, 0, 0}, 0},
        {{0, 0, 1}, 0},
        {{0, 1, 0}, 1},
        {{0, 1, 1}, 1},
        {{1, 0, 0}, 2},
        {{1, 0, 1}, 2},
        {{1, 1, 0}, 3},
        {{1, 1, 1}, 4}
    };
}

} // namespace

TYST_TEST(EndgameTableCompressionTest, DecisionDagPreservesExactOutcomes) {
    DecisionDagTable table;
    table.build(make_small_wcdbl_table(), 3);

    TYST_EXPECT_EQ(table.query({0, 0, 0}), 0);
    TYST_EXPECT_EQ(table.query({0, 1, 1}), 1);
    TYST_EXPECT_EQ(table.query({1, 0, 1}), 2);
    TYST_EXPECT_EQ(table.query({1, 1, 0}), 3);
    TYST_EXPECT_TRUE(table.node_count() < 15u);
}

TYST_TEST(EndgameTableCompressionTest, MtBddConcretizesDontCareTerminals) {
    const std::vector<EndgameEntry> partial = {
        {{0, 0}, 1},
        {{0, 1}, 1},
        {{1, 0}, 2}
    };

    MultiterminalDecisionDiagram table;
    table.build(partial, 2, true);

    TYST_EXPECT_EQ(table.query({0, 0}), 1);
    TYST_EXPECT_EQ(table.query({0, 1}), 1);
    TYST_EXPECT_EQ(table.query({1, 0}), 2);
    TYST_EXPECT_EQ(table.query({1, 1}), 2);
}

TYST_TEST(EndgameTableCompressionTest, LogicCubesMergeWithoutChangingBaseline) {
    LogicMinimizedTable table;
    table.build(make_small_wcdbl_table(), 3, 1);

    for (const auto& entry : make_small_wcdbl_table()) {
        TYST_EXPECT_EQ(table.query(entry.bits), entry.outcome);
    }
    TYST_EXPECT_TRUE(table.cubes().size() < make_small_wcdbl_table().size());
}
