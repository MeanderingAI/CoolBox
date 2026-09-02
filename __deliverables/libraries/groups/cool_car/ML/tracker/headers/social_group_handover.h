#ifndef SOCIAL_GROUP_HANDOVER_H
#define SOCIAL_GROUP_HANDOVER_H

#include <cstddef>
#include <vector>

struct TrackObservation {
    int frame = 0;
    int camera = 0;
    double x = 0.0;
    double y = 0.0;
};

struct SocialTracklet {
    int id = 0;
    std::vector<TrackObservation> observations;
};

struct SocialGroupHandoverConfig {
    std::size_t max_groups = 3;
    std::size_t kmeans_iterations = 20;
    double group_penalty = 0.05;
    double social_weight = 1.0;
    double invalid_link_cost = 1e9;
};

struct SocialGroupHandoverResult {
    std::vector<int> successors;
    std::vector<int> group_assignments;
    std::size_t group_count = 0;
    double total_cost = 0.0;
};

class SocialGroupHandover {
public:
    explicit SocialGroupHandover(const SocialGroupHandoverConfig& config = SocialGroupHandoverConfig());

    SocialGroupHandoverResult solve(
        const std::vector<SocialTracklet>& tracks,
        const std::vector<std::vector<double>>& link_costs) const;

    std::vector<int> estimate_groups(const std::vector<SocialTracklet>& tracks, std::size_t group_count) const;

private:
    SocialGroupHandoverConfig config_;
};

#endif // SOCIAL_GROUP_HANDOVER_H