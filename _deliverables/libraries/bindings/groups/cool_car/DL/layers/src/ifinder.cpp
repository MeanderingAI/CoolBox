#include "ifinder.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace ifinder {

namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\\': escaped << "\\\\"; break;
            case '"': escaped << "\\\""; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << static_cast<char>(ch); break;
        }
    }
    return escaped.str();
}

double squared_distance(const Point3D& left, const Point3D& right) {
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    const double dz = left.z - right.z;
    return dx * dx + dy * dy + dz * dz;
}

double lane_reference_x(const LaneMarking& marking) {
    if (marking.points.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& point : marking.points) {
        sum += point.x;
    }
    return sum / static_cast<double>(marking.points.size());
}

std::vector<double> lane_boundaries(const std::vector<LaneMarking>& lane_markings, double image_width) {
    if (image_width <= 0.0) {
        throw std::invalid_argument("image_width must be positive");
    }

    std::vector<double> boundaries;
    boundaries.reserve(lane_markings.size() + 2u);
    boundaries.push_back(0.0);
    for (const auto& marking : lane_markings) {
        if (!marking.points.empty()) {
            boundaries.push_back(lane_reference_x(marking));
        }
    }
    boundaries.push_back(image_width);
    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
    return boundaries;
}

void write_object_json(std::ostringstream& output, const ObjectCue& object) {
    output << "{\"track_id\":" << object.track_id
           << ",\"class_label\":\"" << json_escape(object.class_label) << "\""
           << ",\"bbox\":[" << object.box.xmin << ',' << object.box.ymin << ','
           << object.box.xmax << ',' << object.box.ymax << ']'
           << ",\"distance_m\":" << object.distance_m
           << ",\"orientation_yaw_rad\":" << object.orientation_yaw_rad
           << ",\"lane\":" << object.lane
           << ",\"attributes\":{";

    bool first_attribute = true;
    for (const auto& attribute : object.attributes) {
        if (!first_attribute) {
            output << ',';
        }
        first_attribute = false;
        output << '"' << json_escape(attribute.first) << "\":\""
               << json_escape(attribute.second) << '"';
    }
    output << "}}";
}

} // namespace

bool BoundingBox::valid() const {
    return xmax >= xmin && ymax >= ymin;
}

Point2D BoundingBox::bottom_midpoint() const {
    return Point2D{(xmin + xmax) * 0.5, ymax};
}

std::string PromptBlocks::compose() const {
    std::ostringstream prompt;
    prompt << "Key Explanation:\n" << key_explanation << "\n\n"
           << "Step Instructions:\n" << step_instructions << "\n\n"
           << "Peer Instruction:\n" << peer_instruction;
    return prompt.str();
}

int assign_lane(const Point2D& point,
                const std::vector<LaneMarking>& lane_markings,
                double image_width) {
    const auto boundaries = lane_boundaries(lane_markings, image_width);
    if (point.x < boundaries.front() || point.x > boundaries.back()) {
        return -1;
    }

    for (std::size_t i = 0; i + 1u < boundaries.size(); ++i) {
        if (point.x >= boundaries[i] && point.x <= boundaries[i + 1u]) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int assign_object_lane(const BoundingBox& box,
                       const std::vector<LaneMarking>& lane_markings,
                       double image_width) {
    if (!box.valid()) {
        throw std::invalid_argument("object bounding box is invalid");
    }
    return assign_lane(box.bottom_midpoint(), lane_markings, image_width);
}

int assign_ego_lane(double image_width,
                    double image_height,
                    const std::vector<LaneMarking>& lane_markings) {
    return assign_lane(Point2D{image_width * 0.5, image_height}, lane_markings, image_width);
}

double masked_mean_distance(const std::vector<double>& depth_map,
                            const std::vector<unsigned char>& mask,
                            std::size_t width,
                            std::size_t height,
                            const BoundingBox& box) {
    if (depth_map.size() != width * height || mask.size() != width * height) {
        throw std::invalid_argument("depth map and mask sizes must match width * height");
    }
    if (!box.valid()) {
        throw std::invalid_argument("distance bounding box is invalid");
    }

    const std::size_t xmin = static_cast<std::size_t>(std::max(0.0, std::floor(box.xmin)));
    const std::size_t ymin = static_cast<std::size_t>(std::max(0.0, std::floor(box.ymin)));
    const std::size_t xmax = static_cast<std::size_t>(std::min(static_cast<double>(width), std::ceil(box.xmax)));
    const std::size_t ymax = static_cast<std::size_t>(std::min(static_cast<double>(height), std::ceil(box.ymax)));

    double sum = 0.0;
    std::size_t count = 0;
    for (std::size_t y = ymin; y < ymax; ++y) {
        for (std::size_t x = xmin; x < xmax; ++x) {
            const std::size_t index = y * width + x;
            if (mask[index] != 0) {
                sum += depth_map[index];
                ++count;
            }
        }
    }

    if (count == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return sum / static_cast<double>(count);
}

EgoState estimate_ego_state(const std::vector<Point3D>& camera_positions,
                            std::size_t frame_index,
                            const IFinderConfig& config) {
    if (camera_positions.empty() || frame_index >= camera_positions.size()) {
        throw std::out_of_range("frame_index is outside camera position sequence");
    }

    EgoState state;
    const std::size_t window = std::max<std::size_t>(1u, config.motion_window);
    const std::size_t motion_end = std::min(camera_positions.size() - 1u, frame_index + window);
    const std::size_t motion_begin = frame_index > window ? frame_index - window : 0u;
    const double distance = std::sqrt(squared_distance(camera_positions[motion_end], camera_positions[motion_begin]));
    const double divisor = static_cast<double>(std::max<std::size_t>(1u, motion_end - motion_begin));
    state.speed = distance / divisor;
    state.motion = state.speed < config.stopped_speed_threshold ? "Stopped" : "Moving";

    if (frame_index == 0 || frame_index + 1u >= camera_positions.size()) {
        state.turn = "Unknown";
        return state;
    }

    const auto& previous = camera_positions[frame_index - 1u];
    const auto& current = camera_positions[frame_index];
    const auto& next = camera_positions[frame_index + 1u];
    const double heading_in = std::atan2(current.z - previous.z, current.x - previous.x);
    const double heading_out = std::atan2(next.z - current.z, next.x - current.x);
    state.heading_delta_rad = heading_out - heading_in;

    if (std::abs(state.heading_delta_rad) < config.turn_threshold_rad) {
        state.turn = "Straight";
    } else if (state.heading_delta_rad > 0.0) {
        state.turn = "Right Turn";
    } else {
        state.turn = "Left Turn";
    }
    return state;
}

std::string summarize_trajectory(const std::vector<FrameCue>& frames, int track_id) {
    const ObjectCue* first = nullptr;
    const ObjectCue* last = nullptr;
    std::size_t first_frame = 0;
    std::size_t last_frame = 0;

    for (const auto& frame : frames) {
        for (const auto& object : frame.detected_objects) {
            if (object.track_id == track_id) {
                if (first == nullptr) {
                    first = &object;
                    first_frame = frame.frame_index;
                }
                last = &object;
                last_frame = frame.frame_index;
            }
        }
    }

    if (first == nullptr || last == nullptr) {
        return "Object ID " + std::to_string(track_id) + " was not observed.";
    }

    std::ostringstream summary;
    summary << "Object ID " << track_id << " (" << first->class_label << ") moved from lane "
            << first->lane << " at frame " << first_frame << " to lane " << last->lane
            << " at frame " << last_frame << ", with distance " << first->distance_m
            << "m to " << last->distance_m << "m and yaw " << first->orientation_yaw_rad
            << " to " << last->orientation_yaw_rad << ".";
    return summary.str();
}

std::string to_json(const VideoCue& cue) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(3);
    output << "{\"Video-Level-Information\":{";
    output << "\"surrounding-info\":\"" << json_escape(cue.surrounding_info) << "\",";
    output << "\"description\":\"" << json_escape(cue.event_description) << "\",";
    output << "\"response\":\"" << json_escape(cue.peer_response) << "\"";
    output << "},\"Frame-Level-Information\":[";

    for (std::size_t i = 0; i < cue.frames.size(); ++i) {
        if (i != 0) {
            output << ',';
        }
        const auto& frame = cue.frames[i];
         output << "{\"frame_index\":" << frame.frame_index
             << ",\"ego-car-information\":{"
               << "\"motion\":\"" << json_escape(frame.ego_state.motion) << "\",";
        output << "\"turn\":\"" << json_escape(frame.ego_state.turn) << "\",";
        output << "\"speed\":" << frame.ego_state.speed << ',';
        output << "\"heading_delta_rad\":" << frame.ego_state.heading_delta_rad << ',';
        output << "\"lane\":" << frame.ego_lane << "},\"detected_objects\":[";

        for (std::size_t j = 0; j < frame.detected_objects.size(); ++j) {
            if (j != 0) {
                output << ',';
            }
            write_object_json(output, frame.detected_objects[j]);
        }
        output << "]}";
    }

    output << "]}";
    return output.str();
}

PromptBlocks build_prompt_blocks(const VideoCue& cue, const std::string& user_query) {
    PromptBlocks blocks;
    blocks.key_explanation =
        "Use the structured dash-cam evidence as the grounding source. Video-level fields describe "
        "global scene context, event summary, and the peer model response. Frame-level fields describe "
        "ego motion, ego turn, ego lane, tracked objects, object lanes, distances, attributes, and yaw.";

    std::ostringstream steps;
    steps << "Answer the user query: \"" << user_query << "\". "
          << "First inspect the global context, then compare the peer response against frame evidence. "
          << "Track object IDs over time, prioritize orientation and distance changes for causal events, "
          << "and cite frame indices or object IDs when they support the conclusion. The structured input has "
          << cue.frames.size() << " frame records.";
    blocks.step_instructions = steps.str();

    blocks.peer_instruction =
        "Treat the peer V-VLM response as an initial hypothesis, not as ground truth. Correct it when "
        "object tracks, lane assignments, ego state, distance, or orientation provide stronger evidence.";
    return blocks;
}

} // namespace ifinder
} // namespace deep_learning
} // namespace ml