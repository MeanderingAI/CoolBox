#include "liquid_ui.hpp"

#include <algorithm>
#include <cmath>

namespace app_assets {
namespace liquid_ui {
namespace {

constexpr float k_pi = 3.14159265358979323846f;

int clamp_i(int value, int low, int high) {
    return std::max(low, std::min(high, value));
}

Rgb blend_over(const Rgb& under, const Rgb& over, float opacity) {
    const float a = std::max(0.0f, std::min(1.0f, opacity));
    const float inv = 1.0f - a;
    return Rgb{
        clamp_i(static_cast<int>(under.r * inv + over.r * a), 0, 255),
        clamp_i(static_cast<int>(under.g * inv + over.g * a), 0, 255),
        clamp_i(static_cast<int>(under.b * inv + over.b * a), 0, 255),
    };
}

} // namespace

void step_scene(LiquidScene& scene, float dt_seconds) {
    scene.elapsed_seconds += std::max(0.0f, dt_seconds);
}

Rect animated_bounds(const LiquidElement& element, float elapsed_seconds) {
    const float drift_phase = 2.0f * k_pi * element.drift_speed_hz * elapsed_seconds + element.phase_offset;
    const float bob_phase = 2.0f * k_pi * element.bob_speed_hz * elapsed_seconds + (element.phase_offset * 0.7f);

    const int x_offset = static_cast<int>(std::sin(drift_phase) * element.drift_amplitude_px);
    const int y_offset = static_cast<int>(std::cos(bob_phase) * element.bob_amplitude_px);

    return Rect{
        element.anchor_bounds.left + x_offset,
        element.anchor_bounds.top + y_offset,
        element.anchor_bounds.right + x_offset,
        element.anchor_bounds.bottom + y_offset,
    };
}

void draw_liquid_panel(graphics::full_application_window::FullApplicationWindow& window,
                       const Rect& bounds,
                       const Rgb& under_color,
                       const Rgb& tint,
                       float opacity,
                       float phase) {
    const Rgb shell = blend_over(under_color, tint, opacity);
    const Rgb shell_border = blend_over(under_color, Rgb{tint.r + 24, tint.g + 24, tint.b + 24}, std::min(1.0f, opacity + 0.1f));

    window.draw_rounded_rect(bounds.left,
                             bounds.top,
                             bounds.right,
                             bounds.bottom,
                             14,
                             clamp_i(shell_border.r, 0, 255),
                             clamp_i(shell_border.g, 0, 255),
                             clamp_i(shell_border.b, 0, 255),
                             false);

    window.draw_rounded_rect(bounds.left + 1,
                             bounds.top + 1,
                             bounds.right - 1,
                             bounds.bottom - 1,
                             13,
                             shell.r,
                             shell.g,
                             shell.b,
                             true);

    const int w = std::max(1, bounds.right - bounds.left);
    const int h = std::max(1, bounds.bottom - bounds.top);
    const int wave_band = std::max(8, h / 4);
    const int wave_y = bounds.top + (h / 2) + static_cast<int>(std::sin(phase) * (h / 8));
    const int shimmer_x = bounds.left + (w / 2) + static_cast<int>(std::sin(phase * 1.7f) * (w / 5));

    const Rgb wave = blend_over(shell, Rgb{164, 214, 255}, 0.22f);
    window.fill_rect(bounds.left + 2,
                     wave_y - (wave_band / 2),
                     bounds.right - 2,
                     wave_y + (wave_band / 2),
                     wave.r,
                     wave.g,
                     wave.b);

    const Rgb shimmer = blend_over(shell, Rgb{232, 248, 255}, 0.28f);
    window.fill_rect(shimmer_x - 8,
                     bounds.top + 4,
                     shimmer_x + 8,
                     bounds.bottom - 4,
                     shimmer.r,
                     shimmer.g,
                     shimmer.b);
}

void draw_liquid_element(graphics::full_application_window::FullApplicationWindow& window,
                         const LiquidElement& element,
                         float elapsed_seconds,
                         const Rgb& under_color) {
    const Rect b = animated_bounds(element, elapsed_seconds);
    const float phase = (2.0f * k_pi * elapsed_seconds * element.drift_speed_hz) + element.phase_offset;

    Rgb tint = element.selected ? Rgb{72, 142, 214} : Rgb{64, 112, 172};
    if (element.selected) {
        tint = blend_over(tint, Rgb{86, 198, 255}, 0.28f);
    }

    draw_liquid_panel(window, b, under_color, tint, element.opacity, phase);
    window.draw_text_line(b.left + 12, b.top + ((b.bottom - b.top) / 2) - 7, element.label, 235, 243, 252);
}

void draw_scene(graphics::full_application_window::FullApplicationWindow& window,
                const LiquidScene& scene) {
    for (const auto& element : scene.elements) {
        draw_liquid_element(window, element, scene.elapsed_seconds, scene.under_color);
    }
}

} // namespace liquid_ui
} // namespace app_assets
