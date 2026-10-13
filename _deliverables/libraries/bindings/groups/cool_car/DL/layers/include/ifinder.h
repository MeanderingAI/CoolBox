#ifndef IFINDER_H
#define IFINDER_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace ifinder {

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct BoundingBox {
    double xmin = 0.0;
    double ymin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;

    bool valid() const;
    Point2D bottom_midpoint() const;
};

struct LaneMarking {
    std::vector<Point2D> points;
};

struct IFinderConfig {
    double turn_threshold_rad = 0.08;
    double stopped_speed_threshold = 0.05;
    std::size_t motion_window = 1;
};

struct EgoState {
    std::string motion = "Unknown";
    std::string turn = "Unknown";
    double speed = 0.0;
    double heading_delta_rad = 0.0;
};

struct ObjectCue {
    int track_id = -1;
    std::string class_label;
    BoundingBox box;
    double distance_m = -1.0;
    double orientation_yaw_rad = 0.0;
    int lane = -1;
    std::map<std::string, std::string> attributes;
};

struct FrameCue {
    std::size_t frame_index = 0;
    EgoState ego_state;
    int ego_lane = -1;
    std::vector<ObjectCue> detected_objects;
};

struct VideoCue {
    std::string surrounding_info;
    std::string event_description;
    std::string peer_response;
    std::vector<FrameCue> frames;
};

struct PromptBlocks {
    std::string key_explanation;
    std::string step_instructions;
    std::string peer_instruction;

    std::string compose() const;
};

int assign_lane(const Point2D& point,
                const std::vector<LaneMarking>& lane_markings,
                double image_width);
int assign_object_lane(const BoundingBox& box,
                       const std::vector<LaneMarking>& lane_markings,
                       double image_width);
int assign_ego_lane(double image_width,
                    double image_height,
                    const std::vector<LaneMarking>& lane_markings);

double masked_mean_distance(const std::vector<double>& depth_map,
                            const std::vector<unsigned char>& mask,
                            std::size_t width,
                            std::size_t height,
                            const BoundingBox& box);

EgoState estimate_ego_state(const std::vector<Point3D>& camera_positions,
                            std::size_t frame_index,
                            const IFinderConfig& config = IFinderConfig());

std::string summarize_trajectory(const std::vector<FrameCue>& frames, int track_id);
std::string to_json(const VideoCue& cue);
PromptBlocks build_prompt_blocks(const VideoCue& cue, const std::string& user_query);

} // namespace ifinder
} // namespace deep_learning
} // namespace ml

#endif // IFINDER_H