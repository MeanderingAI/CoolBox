#ifndef COOLBOX_APP_ASSETS_IOS_CHROME_HPP
#define COOLBOX_APP_ASSETS_IOS_CHROME_HPP

#include "full_application_window.hpp"

#include <string>
#include <vector>

namespace app_assets {
namespace ios_chrome {

struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

bool contains(const Rect& rect, int x, int y);

enum class ButtonTone {
    Neutral,
    Primary,
    Success,
    Danger,
};

struct ButtonSpec {
    Rect bounds;
    std::string label;
    ButtonTone tone = ButtonTone::Neutral;
    bool enabled = true;
    bool hovered = false;
    bool selected = false;
};

void draw_button_group(graphics::full_application_window::FullApplicationWindow& window,
                       const Rect& bounds);

void draw_ios_button(graphics::full_application_window::FullApplicationWindow& window,
                     const ButtonSpec& spec);

struct SegmentedControlSpec {
    Rect bounds;
    std::vector<std::string> segments;
    int selected_index = 0;
    int hovered_index = -1;
    bool enabled = true;
};

void draw_segmented_control(graphics::full_application_window::FullApplicationWindow& window,
                            const SegmentedControlSpec& spec);

int segmented_index_at(const SegmentedControlSpec& spec, int x, int y);

struct ToggleChipSpec {
    Rect bounds;
    std::string label;
    bool on = false;
    bool hovered = false;
    bool enabled = true;
};

void draw_toggle_chip(graphics::full_application_window::FullApplicationWindow& window,
                      const ToggleChipSpec& spec);

namespace typography {

const std::vector<int>& standard_font_sizes();
const std::vector<std::string>& standard_font_families();

} // namespace typography

} // namespace ios_chrome
} // namespace app_assets

#endif // COOLBOX_APP_ASSETS_IOS_CHROME_HPP
