#ifndef COOLBOX_APP_ASSETS_LIQUID_UI_HPP
#define COOLBOX_APP_ASSETS_LIQUID_UI_HPP

#include "full_application_window.hpp"

#include <string>
#include <vector>

namespace app_assets {
namespace liquid_ui {

struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct Rgb {
    int r = 0;
    int g = 0;
    int b = 0;
};

struct LiquidElement {
    Rect anchor_bounds;
    std::string label;
    float drift_amplitude_px = 8.0f;
    float bob_amplitude_px = 4.0f;
    float drift_speed_hz = 0.28f;
    float bob_speed_hz = 0.44f;
    float phase_offset = 0.0f;
    float opacity = 0.55f;
    bool selected = false;
};

struct LiquidScene {
    std::vector<LiquidElement> elements;
    float elapsed_seconds = 0.0f;
    Rgb under_color{20, 26, 36};
};

void step_scene(LiquidScene& scene, float dt_seconds);

Rect animated_bounds(const LiquidElement& element, float elapsed_seconds);

void draw_liquid_panel(graphics::full_application_window::FullApplicationWindow& window,
                       const Rect& bounds,
                       const Rgb& under_color,
                       const Rgb& tint,
                       float opacity,
                       float phase);

void draw_liquid_element(graphics::full_application_window::FullApplicationWindow& window,
                         const LiquidElement& element,
                         float elapsed_seconds,
                         const Rgb& under_color);

void draw_scene(graphics::full_application_window::FullApplicationWindow& window,
                const LiquidScene& scene);

} // namespace liquid_ui
} // namespace app_assets

#endif // COOLBOX_APP_ASSETS_LIQUID_UI_HPP
