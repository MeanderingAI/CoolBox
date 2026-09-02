#include "social_group_handover.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {

struct TrackSummary {
    int start_frame = 0;
    int end_frame = 0;
    int start_camera = 0;
    int end_camera = 0;
    double start_x = 0.0;
    double start_y = 0.0;
    double end_x = 0.0;
    double end_y = 0.0;
};

TrackSummary summarize(const SocialTracklet& track) {
    if (track.observations.empty()) {
        throw std::invalid_argument("SocialGroupHandover requires non-empty tracklets");
    }
    auto first = std::min_element(track.observations.begin(), track.observations.end(), [](const auto& left, const auto& right) {
        return left.frame < right.frame;
    });
    auto last = std::max_element(track.observations.begin(), track.observations.end(), [](const auto& left, const auto& right) {
        return left.frame < right.frame;
    });
    return {first->frame, last->frame, first->camera, last->camera, first->x, first->y, last->x, last->y};
}

double squared_distance(double ax, double ay, double bx, double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx * dx + dy * dy;
}

std::vector<double> descriptor(const TrackSummary& summary) {
    return {
        (summary.start_x + summary.end_x) * 0.5,
        (summary.start_y + summary.end_y) * 0.5,
        static_cast<double>(summary.start_camera + summary.end_camera) * 0.5
    };
}

double descriptor_distance(const std::vector<double>& left, const std::vector<double>& right) {
    double total = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const double delta = left[index] - right[index];
        total += delta * delta;
    }
    return total;
}

std::pair<double, double> group_end_centroid(
    const std::vector<TrackSummary>& summaries,
    const std::vector<int>& groups,
    int group,
    int camera) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    int count = 0;
    for (std::size_t index = 0; index < summaries.size(); ++index) {
        if (groups[index] == group && summaries[index].end_camera == camera) {
            sum_x += summaries[index].end_x;
            sum_y += summaries[index].end_y;
            ++count;
        }
    }
    if (count == 0) return {0.0, 0.0};
    return {sum_x / static_cast<double>(count), sum_y / static_cast<double>(count)};
}

std::pair<double, double> group_start_centroid(
    const std::vector<TrackSummary>& summaries,
    const std::vector<int>& groups,
    int group,
    int camera) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    int count = 0;
    for (std::size_t index = 0; index < summaries.size(); ++index) {
        if (groups[index] == group && summaries[index].start_camera == camera) {
            sum_x += summaries[index].start_x;
            sum_y += summaries[index].start_y;
            ++count;
        }
    }
    if (count == 0) return {0.0, 0.0};
    return {sum_x / static_cast<double>(count), sum_y / static_cast<double>(count)};
}

double social_link_cost(
    const std::vector<TrackSummary>& summaries,
    const std::vector<int>& groups,
    std::size_t source,
    std::size_t target) {
    if (groups[source] != groups[target]) return 1000.0;
    const int group = groups[source];
    const auto source_center = group_end_centroid(summaries, groups, group, summaries[source].end_camera);
    const auto target_center = group_start_centroid(summaries, groups, group, summaries[target].start_camera);
    const double source_offset_x = summaries[source].end_x - source_center.first;
    const double source_offset_y = summaries[source].end_y - source_center.second;
    const double target_offset_x = summaries[target].start_x - target_center.first;
    const double target_offset_y = summaries[target].start_y - target_center.second;
    return squared_distance(source_offset_x, source_offset_y, target_offset_x, target_offset_y);
}

} // namespace

SocialGroupHandover::SocialGroupHandover(const SocialGroupHandoverConfig& config)
    : config_(config) {}

std::vector<int> SocialGroupHandover::estimate_groups(const std::vector<SocialTracklet>& tracks, std::size_t group_count) const {
    if (tracks.empty()) return {};
    group_count = std::max<std::size_t>(1, std::min(group_count, tracks.size()));
    std::vector<std::vector<double>> descriptors;
    descriptors.reserve(tracks.size());
    for (const auto& track : tracks) descriptors.push_back(descriptor(summarize(track)));

    std::vector<std::vector<double>> centers;
    centers.reserve(group_count);
    for (std::size_t group = 0; group < group_count; ++group) {
        centers.push_back(descriptors[group * descriptors.size() / group_count]);
    }

    std::vector<int> assignments(tracks.size(), 0);
    for (std::size_t iteration = 0; iteration < config_.kmeans_iterations; ++iteration) {
        for (std::size_t track = 0; track < descriptors.size(); ++track) {
            double best_distance = std::numeric_limits<double>::infinity();
            int best_group = 0;
            for (std::size_t group = 0; group < centers.size(); ++group) {
                const double distance = descriptor_distance(descriptors[track], centers[group]);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_group = static_cast<int>(group);
                }
            }
            assignments[track] = best_group;
        }

        std::vector<std::vector<double>> next_centers(centers.size(), std::vector<double>(descriptors.front().size(), 0.0));
        std::vector<int> counts(centers.size(), 0);
        for (std::size_t track = 0; track < descriptors.size(); ++track) {
            ++counts[assignments[track]];
            for (std::size_t dim = 0; dim < descriptors[track].size(); ++dim) {
                next_centers[assignments[track]][dim] += descriptors[track][dim];
            }
        }
        for (std::size_t group = 0; group < centers.size(); ++group) {
            if (counts[group] == 0) continue;
            for (double& value : next_centers[group]) value /= static_cast<double>(counts[group]);
            centers[group] = next_centers[group];
        }
    }
    return assignments;
}

SocialGroupHandoverResult SocialGroupHandover::solve(
    const std::vector<SocialTracklet>& tracks,
    const std::vector<std::vector<double>>& link_costs) const {
    if (tracks.empty() || link_costs.size() != tracks.size()) {
        throw std::invalid_argument("SocialGroupHandover requires a square link cost matrix");
    }
    for (const auto& row : link_costs) {
        if (row.size() != tracks.size()) throw std::invalid_argument("SocialGroupHandover requires a square link cost matrix");
    }

    std::vector<TrackSummary> summaries;
    summaries.reserve(tracks.size());
    for (const auto& track : tracks) summaries.push_back(summarize(track));

    SocialGroupHandoverResult best;
    best.total_cost = std::numeric_limits<double>::infinity();

    const std::size_t max_groups = std::max<std::size_t>(1, std::min(config_.max_groups, tracks.size()));
    for (std::size_t group_count = 1; group_count <= max_groups; ++group_count) {
        const std::vector<int> groups = estimate_groups(tracks, group_count);
        std::vector<int> successors(tracks.size(), -1);
        std::vector<bool> used_targets(tracks.size(), false);
        double total_cost = config_.group_penalty * static_cast<double>(group_count);

        for (std::size_t source = 0; source < tracks.size(); ++source) {
            double best_link_cost = config_.invalid_link_cost;
            int best_target = -1;
            for (std::size_t target = 0; target < tracks.size(); ++target) {
                if (source == target || used_targets[target]) continue;
                if (summaries[source].end_frame > summaries[target].start_frame) continue;
                const double cost = link_costs[source][target]
                    + config_.social_weight * social_link_cost(summaries, groups, source, target);
                if (cost < best_link_cost) {
                    best_link_cost = cost;
                    best_target = static_cast<int>(target);
                }
            }
            if (best_target >= 0 && best_link_cost < config_.invalid_link_cost) {
                successors[source] = best_target;
                used_targets[static_cast<std::size_t>(best_target)] = true;
                total_cost += best_link_cost;
            }
        }

        if (total_cost < best.total_cost) {
            best.successors = successors;
            best.group_assignments = groups;
            best.group_count = group_count;
            best.total_cost = total_cost;
        }
    }

    return best;
}