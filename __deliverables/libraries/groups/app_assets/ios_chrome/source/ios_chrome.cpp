#include "ios_chrome.hpp"

#include <algorithm>

namespace app_assets {
namespace ios_chrome {
namespace {

struct Rgb {
    int r;
    int g;
    int b;
};

Rgb clamp_rgb(const Rgb& c) {
    return Rgb{
        std::clamp(c.r, 0, 255),
        std::clamp(c.g, 0, 255),
        std::clamp(c.b, 0, 255),
    };
}

Rgb add_rgb(const Rgb& c, int delta) {
    return clamp_rgb(Rgb{c.r + delta, c.g + delta, c.b + delta});
}

Rgb scale_rgb(const Rgb& c, float factor) {
    return clamp_rgb(Rgb{
        static_cast<int>(static_cast<float>(c.r) * factor),
        static_cast<int>(static_cast<float>(c.g) * factor),
        static_cast<int>(static_cast<float>(c.b) * factor),
    });
}

Rgb tone_fill(ButtonTone tone) {
    switch (tone) {
        case ButtonTone::Primary: return Rgb{27, 114, 226};
        case ButtonTone::Success: return Rgb{40, 153, 93};
        case ButtonTone::Danger: return Rgb{199, 73, 68};
        case ButtonTone::Neutral:
        default: return Rgb{90, 100, 122};
    }
}

} // namespace

bool contains(const Rect& rect, int x, int y) {
    return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
}

void draw_button_group(graphics::full_application_window::FullApplicationWindow& window,
                       const Rect& bounds) {
    window.draw_rounded_rect(bounds.left,
                             bounds.top,
                             bounds.right,
                             bounds.bottom,
                             16,
                             62,
                             72,
                             89,
                             false);
    window.draw_rounded_rect(bounds.left + 1,
                             bounds.top + 1,
                             bounds.right - 1,
                             bounds.bottom - 1,
                             15,
                             31,
                             38,
                             49,
                             true);
}

void draw_ios_button(graphics::full_application_window::FullApplicationWindow& window,
                     const ButtonSpec& spec) {
    Rgb fill = tone_fill(spec.tone);
    if (spec.hovered) {
        fill = add_rgb(fill, 12);
    }
    if (spec.selected) {
        fill = add_rgb(fill, 20);
    }
    if (!spec.enabled) {
        fill = scale_rgb(fill, 0.55f);
    }

    const Rgb border = spec.enabled ? add_rgb(fill, -26) : add_rgb(fill, -14);
    const Rgb inner = add_rgb(fill, 10);
    const Rgb text = spec.enabled ? Rgb{245, 247, 251} : Rgb{194, 200, 211};

    window.draw_rounded_rect(spec.bounds.left,
                             spec.bounds.top,
                             spec.bounds.right,
                             spec.bounds.bottom,
                             14,
                             border.r,
                             border.g,
                             border.b,
                             false);
    window.draw_rounded_rect(spec.bounds.left + 1,
                             spec.bounds.top + 1,
                             spec.bounds.right - 1,
                             spec.bounds.bottom - 1,
                             13,
                             fill.r,
                             fill.g,
                             fill.b,
                             true);

    const int shine_bottom = spec.bounds.top +
                             std::max(2, ((spec.bounds.bottom - spec.bounds.top) / 2) - 2);
    window.draw_rounded_rect(spec.bounds.left + 2,
                             spec.bounds.top + 2,
                             spec.bounds.right - 2,
                             shine_bottom,
                             11,
                             inner.r,
                             inner.g,
                             inner.b,
                             true);

    const int text_x = spec.bounds.left + 14;
    const int text_y = spec.bounds.top + ((spec.bounds.bottom - spec.bounds.top) / 2) - 7;
    window.draw_text_line(text_x, text_y, spec.label, text.r, text.g, text.b);
}

void draw_segmented_control(graphics::full_application_window::FullApplicationWindow& window,
                            const SegmentedControlSpec& spec) {
    if (spec.segments.empty()) {
        return;
    }

    const int segment_count = static_cast<int>(spec.segments.size());
    const int width = std::max(1, spec.bounds.right - spec.bounds.left);
    const int segment_width = std::max(1, width / segment_count);

    window.draw_rounded_rect(spec.bounds.left,
                             spec.bounds.top,
                             spec.bounds.right,
                             spec.bounds.bottom,
                             12,
                             68,
                             78,
                             96,
                             false);
    window.draw_rounded_rect(spec.bounds.left + 1,
                             spec.bounds.top + 1,
                             spec.bounds.right - 1,
                             spec.bounds.bottom - 1,
                             11,
                             36,
                             44,
                             56,
                             true);

    for (int i = 0; i < segment_count; ++i) {
        const int left = spec.bounds.left + (i * segment_width);
        const int right = (i == segment_count - 1) ? spec.bounds.right : (left + segment_width);
        const bool selected = i == spec.selected_index;
        const bool hovered = i == spec.hovered_index;

        Rgb fill{72, 84, 104};
        if (selected) {
            fill = Rgb{39, 130, 244};
        }
        if (hovered) {
            fill = add_rgb(fill, 10);
        }
        if (!spec.enabled) {
            fill = scale_rgb(fill, 0.55f);
        }

        window.fill_rect(left + 1, spec.bounds.top + 1, right - 1, spec.bounds.bottom - 1, fill.r, fill.g, fill.b);

        if (i > 0) {
            window.fill_rect(left, spec.bounds.top + 4, left + 1, spec.bounds.bottom - 4, 95, 109, 133);
        }

        const Rgb text = selected ? Rgb{248, 250, 252} : Rgb{214, 222, 234};
        window.draw_text_line(left + 10,
                              spec.bounds.top + ((spec.bounds.bottom - spec.bounds.top) / 2) - 7,
                              spec.segments[static_cast<std::size_t>(i)],
                              text.r,
                              text.g,
                              text.b);
    }
}

int segmented_index_at(const SegmentedControlSpec& spec, int x, int y) {
    if (!contains(spec.bounds, x, y) || spec.segments.empty()) {
        return -1;
    }

    const int segment_count = static_cast<int>(spec.segments.size());
    const int width = std::max(1, spec.bounds.right - spec.bounds.left);
    const int segment_width = std::max(1, width / segment_count);
    const int rel_x = std::clamp(x - spec.bounds.left, 0, width - 1);
    const int index = rel_x / segment_width;
    return std::clamp(index, 0, segment_count - 1);
}

void draw_toggle_chip(graphics::full_application_window::FullApplicationWindow& window,
                      const ToggleChipSpec& spec) {
    Rgb fill = spec.on ? Rgb{38, 150, 92} : Rgb{84, 96, 118};
    if (spec.hovered) {
        fill = add_rgb(fill, 10);
    }
    if (!spec.enabled) {
        fill = scale_rgb(fill, 0.55f);
    }

    const Rgb border = add_rgb(fill, -22);
    const Rgb text = spec.enabled ? Rgb{246, 248, 252} : Rgb{188, 196, 208};

    window.draw_rounded_rect(spec.bounds.left,
                             spec.bounds.top,
                             spec.bounds.right,
                             spec.bounds.bottom,
                             11,
                             border.r,
                             border.g,
                             border.b,
                             false);
    window.draw_rounded_rect(spec.bounds.left + 1,
                             spec.bounds.top + 1,
                             spec.bounds.right - 1,
                             spec.bounds.bottom - 1,
                             10,
                             fill.r,
                             fill.g,
                             fill.b,
                             true);

    const std::string chip_label = spec.label + (spec.on ? " ON" : " OFF");
    window.draw_text_line(spec.bounds.left + 10,
                          spec.bounds.top + ((spec.bounds.bottom - spec.bounds.top) / 2) - 7,
                          chip_label,
                          text.r,
                          text.g,
                          text.b);
}

namespace typography {

const std::vector<int>& standard_font_sizes() {
    static const std::vector<int> k_sizes = {12, 14, 16, 18, 20, 24, 28};
    return k_sizes;
}

const std::vector<std::string>& standard_font_families() {
    static const std::vector<std::string> k_families = {"Default", "Monospace", "Sans"};
    return k_families;
}

} // namespace typography

} // namespace ios_chrome
} // namespace app_assets
