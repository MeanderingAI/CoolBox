#include "tyst_framework.hpp"

#include "social_group_handover.h"

#include <vector>

namespace {

SocialTracklet make_track(int id, int start_frame, int camera, double x, double y) {
    SocialTracklet track;
    track.id = id;
    track.observations.push_back({start_frame, camera, x, y});
    track.observations.push_back({start_frame + 2, camera, x, y});
    return track;
}

} // namespace

TYST_TEST(SocialGroupHandoverTest, SocialOffsetsResolveAmbiguousAppearanceHandover) {
    const std::vector<SocialTracklet> tracks = {
        make_track(0, 0, 0, -1.0, 0.0),
        make_track(1, 0, 0, 1.0, 0.0),
        make_track(2, 10, 1, -1.0, 0.0),
        make_track(3, 10, 1, 1.0, 0.0)
    };

    std::vector<std::vector<double>> link_costs(4, std::vector<double>(4, 1000.0));
    link_costs[0][2] = 0.4;
    link_costs[0][3] = 0.1;
    link_costs[1][2] = 0.1;
    link_costs[1][3] = 0.4;

    SocialGroupHandoverConfig config;
    config.max_groups = 1;
    config.social_weight = 2.0;
    SocialGroupHandover handover(config);
    const SocialGroupHandoverResult result = handover.solve(tracks, link_costs);

    TYST_EXPECT_EQ(result.successors[0], 2);
    TYST_EXPECT_EQ(result.successors[1], 3);
    TYST_EXPECT_EQ(result.group_count, static_cast<std::size_t>(1));
}

TYST_TEST(SocialGroupHandoverTest, EstimatesGroupsFromTrackGeometry) {
    const std::vector<SocialTracklet> tracks = {
        make_track(0, 0, 0, -10.0, 0.0),
        make_track(1, 1, 0, -9.5, 0.0),
        make_track(2, 0, 0, 10.0, 0.0),
        make_track(3, 1, 0, 10.5, 0.0)
    };

    SocialGroupHandover handover;
    const std::vector<int> groups = handover.estimate_groups(tracks, 2);

    TYST_EXPECT_EQ(groups.size(), static_cast<std::size_t>(4));
    TYST_EXPECT_EQ(groups[0], groups[1]);
    TYST_EXPECT_EQ(groups[2], groups[3]);
    TYST_EXPECT_TRUE(groups[0] != groups[2]);
}