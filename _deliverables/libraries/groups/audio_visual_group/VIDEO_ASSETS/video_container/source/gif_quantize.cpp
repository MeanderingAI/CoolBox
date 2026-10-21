// Median-cut color quantizer used to reduce an RGB24 VideoFrame down to the
// small (<=256 entry) palette required by GIF's indexed color model.
#include "../headers/video_container.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>

namespace trekker {
namespace video {
namespace container {

namespace {

struct ColorCount {
    std::uint8_t r, g, b;
    std::uint64_t count;
};

struct ColorBox {
    std::vector<ColorCount> colors; // contiguous slice owned by the box

    std::array<std::uint8_t, 3> channel_range() const {
        std::uint8_t lo[3] = {255, 255, 255};
        std::uint8_t hi[3] = {0, 0, 0};
        for (const auto& c : colors) {
            const std::uint8_t v[3] = {c.r, c.g, c.b};
            for (int ch = 0; ch < 3; ++ch) {
                lo[ch] = std::min(lo[ch], v[ch]);
                hi[ch] = std::max(hi[ch], v[ch]);
            }
        }
        return {static_cast<std::uint8_t>(hi[0] - lo[0]),
                static_cast<std::uint8_t>(hi[1] - lo[1]),
                static_cast<std::uint8_t>(hi[2] - lo[2])};
    }

    int longest_axis() const {
        const auto range = channel_range();
        int axis = 0;
        if (range[1] > range[axis]) axis = 1;
        if (range[2] > range[axis]) axis = 2;
        return axis;
    }

    std::uint64_t total_count() const {
        std::uint64_t sum = 0;
        for (const auto& c : colors) sum += c.count;
        return sum;
    }
};

std::uint8_t channel_value(const ColorCount& c, int axis) {
    return axis == 0 ? c.r : (axis == 1 ? c.g : c.b);
}

// Splits `box` in place along its longest axis at the (count-weighted)
// median, returning the newly created second half.
ColorBox split_box(ColorBox& box) {
    const int axis = box.longest_axis();
    std::sort(box.colors.begin(), box.colors.end(),
              [axis](const ColorCount& a, const ColorCount& b) {
                  return channel_value(a, axis) < channel_value(b, axis);
              });

    const std::uint64_t total = box.total_count();
    std::uint64_t running = 0;
    std::size_t split_index = box.colors.size() / 2; // fallback: even split
    for (std::size_t i = 0; i < box.colors.size(); ++i) {
        running += box.colors[i].count;
        if (running * 2 >= total) {
            split_index = i + 1;
            break;
        }
    }
    split_index = std::max<std::size_t>(1, std::min(split_index, box.colors.size() - 1));

    ColorBox second;
    second.colors.assign(box.colors.begin() + static_cast<long>(split_index), box.colors.end());
    box.colors.resize(split_index);
    return second;
}

std::array<std::uint8_t, 3> average_color(const ColorBox& box) {
    std::uint64_t r = 0, g = 0, b = 0, total = 0;
    for (const auto& c : box.colors) {
        r += static_cast<std::uint64_t>(c.r) * c.count;
        g += static_cast<std::uint64_t>(c.g) * c.count;
        b += static_cast<std::uint64_t>(c.b) * c.count;
        total += c.count;
    }
    if (total == 0) return {0, 0, 0};
    return {static_cast<std::uint8_t>(r / total),
            static_cast<std::uint8_t>(g / total),
            static_cast<std::uint8_t>(b / total)};
}

inline std::uint32_t pack_rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) | b;
}

} // namespace

QuantizedImage median_cut_quantize(const VideoFrame& rgb_frame_in, int max_colors) {
    max_colors = std::max(2, std::min(256, max_colors));

    VideoFrame rgb_frame = rgb_frame_in.format == PixelFormat::RGB24
                               ? rgb_frame_in
                               : convert_format(rgb_frame_in, PixelFormat::RGB24);

    const std::size_t w = rgb_frame.width;
    const std::size_t h = rgb_frame.height;
    const Plane& plane = rgb_frame.planes[0];

    // Count distinct colors so identical pixels only contribute one weighted
    // sample to the median-cut split decisions.
    std::unordered_map<std::uint32_t, std::uint64_t> histogram;
    histogram.reserve(w * h);
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            const std::uint8_t r = plane.at(x * 3 + 0, y);
            const std::uint8_t g = plane.at(x * 3 + 1, y);
            const std::uint8_t b = plane.at(x * 3 + 2, y);
            ++histogram[pack_rgb(r, g, b)];
        }
    }

    ColorBox root;
    root.colors.reserve(histogram.size());
    for (const auto& [packed, count] : histogram) {
        root.colors.push_back({static_cast<std::uint8_t>((packed >> 16) & 0xFF),
                               static_cast<std::uint8_t>((packed >> 8) & 0xFF),
                               static_cast<std::uint8_t>(packed & 0xFF), count});
    }

    std::vector<ColorBox> boxes;
    boxes.push_back(std::move(root));

    // Repeatedly split the box with the most distinct colors (a simple,
    // effective heuristic) until we hit the palette budget or every box is a
    // single color.
    while (static_cast<int>(boxes.size()) < max_colors) {
        auto best_it = boxes.end();
        std::size_t best_size = 1; // boxes with exactly 1 color cannot split
        for (auto it = boxes.begin(); it != boxes.end(); ++it) {
            if (it->colors.size() > best_size) {
                best_size = it->colors.size();
                best_it = it;
            }
        }
        if (best_it == boxes.end()) break; // nothing left worth splitting
        ColorBox second = split_box(*best_it);
        boxes.push_back(std::move(second));
    }

    QuantizedImage result;
    result.palette.reserve(boxes.size() * 3);
    std::unordered_map<std::uint32_t, std::uint8_t> color_to_index;
    color_to_index.reserve(histogram.size());
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const auto avg = average_color(boxes[i]);
        result.palette.push_back(avg[0]);
        result.palette.push_back(avg[1]);
        result.palette.push_back(avg[2]);
        for (const auto& c : boxes[i].colors) {
            color_to_index[pack_rgb(c.r, c.g, c.b)] = static_cast<std::uint8_t>(i);
        }
    }

    result.indices.resize(w * h);
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            const std::uint8_t r = plane.at(x * 3 + 0, y);
            const std::uint8_t g = plane.at(x * 3 + 1, y);
            const std::uint8_t b = plane.at(x * 3 + 2, y);
            result.indices[y * w + x] = color_to_index.at(pack_rgb(r, g, b));
        }
    }
    return result;
}

} // namespace container
} // namespace video
} // namespace trekker
