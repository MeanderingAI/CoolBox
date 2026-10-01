#include "tyst_framework.hpp"

#include "video_relevance_feedback.h"

#include <cmath>
#include <vector>

using namespace ml::video_retrieval;

namespace {

std::vector<VideoItem> make_database() {
    return {
        {"basketball_dunk", {2.0, 2.1}},
        {"basketball_pass", {1.8, 2.0}},
        {"tennis_swing", {-1.9, -1.7}},
        {"quiet_landscape", {-2.2, -2.0}}
    };
}

std::vector<LabeledVideo> make_target_feedback() {
    return {
        {"query_positive", {1.0, 1.0}, 1},
        {"query_negative", {-1.0, -1.0}, -1}
    };
}

std::vector<SourceTask> make_sources() {
    return {
        {"sports_motion", {
            {"sports_positive_1", {2.0, 1.9}, 1},
            {"sports_positive_2", {1.7, 2.1}, 1},
            {"sports_negative_1", {-1.8, -1.7}, -1},
            {"sports_negative_2", {-2.1, -1.9}, -1}
        }},
        {"inverted_source", {
            {"wrong_positive_1", {-2.0, -1.8}, 1},
            {"wrong_negative_1", {2.0, 1.8}, -1}
        }}
    };
}

} // namespace

TYST_TEST(VideoRelevanceFeedbackTest, ScoreAccuracySelectsRelatedSourceAndRanksVideos) {
    RfTlVideoRanker ranker;
    ranker.fit(make_target_feedback(), make_sources(), make_database(), SourceSelectionMethod::ScoreAccuracy);

    const auto ranked = ranker.rank(make_database());

    TYST_EXPECT_EQ(ranker.selected_source_name(), "sports_motion");
    TYST_EXPECT_EQ(ranked.front().id, "basketball_dunk");
    TYST_EXPECT_TRUE(ranked.front().score > ranked.back().score);
}

TYST_TEST(VideoRelevanceFeedbackTest, FeedbackStrategiesReturnExpectedCandidates) {
    RfTlVideoRanker ranker;
    ranker.fit(make_target_feedback(), make_sources(), make_database(), SourceSelectionMethod::MaxMargin);

    const auto top = ranker.solicit_feedback(make_database(), FeedbackStrategy::TopRanked, 1);
    const auto uncertain = ranker.solicit_feedback(make_database(), FeedbackStrategy::Uncertainty, 2);

    TYST_EXPECT_EQ(top.size(), 1u);
    TYST_EXPECT_EQ(top.front().id, "basketball_dunk");
    TYST_EXPECT_EQ(uncertain.size(), 2u);
    TYST_EXPECT_TRUE(std::abs(ranker.score(uncertain[0].features)) <= std::abs(ranker.score(uncertain[1].features)));
}

TYST_TEST(VideoRelevanceFeedbackTest, ScoreClusteringAndNormalizationAreUsable) {
    const auto normalized = normalize_by_standard_deviation(make_database());

    RfTlVideoRanker ranker;
    ranker.fit(make_target_feedback(), make_sources(), normalized, SourceSelectionMethod::ScoreClustering);

    const auto ranked = ranker.rank(normalized);
    TYST_EXPECT_EQ(ranker.selected_source_name(), "sports_motion");
    TYST_EXPECT_EQ(ranked.front().id, "basketball_dunk");
}
