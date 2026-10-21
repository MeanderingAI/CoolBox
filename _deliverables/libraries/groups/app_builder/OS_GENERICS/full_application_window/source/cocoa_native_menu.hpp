#pragma once

#include "components.hpp"
#include <functional>

namespace graphics::full_application_window::detail {
using CocoaMenuCommand = std::function<void(std::size_t, std::size_t)>;
void* create_cocoa_menu(void* window, const components::MenuBarModel& model,
                        CocoaMenuCommand command);
void destroy_cocoa_menu(void* menu);
}
