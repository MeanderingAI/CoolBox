#include "coolbox/coolbox_c.h"
#include "coolbox/coolbox_xamarin_bindings.h"
#include "liquid_ui.hpp"

namespace {

app_assets::liquid_ui::LiquidElement to_liquid_element(const CoolBoxXamarinLiquidElementSpec *spec) {
    app_assets::liquid_ui::LiquidElement element;
    if (spec == nullptr) {
        return element;
    }

    element.anchor_bounds = app_assets::liquid_ui::Rect{spec->left, spec->top, spec->right, spec->bottom};
    element.drift_amplitude_px = spec->drift_amplitude_px;
    element.bob_amplitude_px = spec->bob_amplitude_px;
    element.drift_speed_hz = spec->drift_speed_hz;
    element.bob_speed_hz = spec->bob_speed_hz;
    element.phase_offset = spec->phase_offset;
    return element;
}

} // namespace

extern "C" {

const char *coolbox_xamarin_version(void) {
    return coolbox_c_version();
}

const char *coolbox_xamarin_default_endpoint(void) {
    return coolbox_c_default_endpoint();
}

size_t coolbox_xamarin_capability_count(void) {
    return coolbox_c_capability_count();
}

const char *coolbox_xamarin_capability_at(size_t index) {
    return coolbox_c_capability_at(index);
}

void coolbox_xamarin_liquid_animated_bounds(
    const CoolBoxXamarinLiquidElementSpec *spec,
    float elapsed_seconds,
    int *out_left,
    int *out_top,
    int *out_right,
    int *out_bottom) {
    const auto element = to_liquid_element(spec);
    const auto bounds = app_assets::liquid_ui::animated_bounds(element, elapsed_seconds);

    if (out_left != nullptr) {
        *out_left = bounds.left;
    }
    if (out_top != nullptr) {
        *out_top = bounds.top;
    }
    if (out_right != nullptr) {
        *out_right = bounds.right;
    }
    if (out_bottom != nullptr) {
        *out_bottom = bounds.bottom;
    }
}

} // extern "C"
