#include "application_context.hpp"
#include <algorithm>

namespace application_context {

ApplicationContext::ApplicationContext(std::string name)
    : name_(std::move(name)) {}

ApplicationContext::~ApplicationContext() = default;

void ApplicationContext::set_icon(::graphics::windows::PlatformStyle platform, const Icon &icon) {
    icons_[platform] = icon;
}

bool ApplicationContext::has_icon(::graphics::windows::PlatformStyle platform) const {
    return icons_.find(platform) != icons_.end();
}

Icon ApplicationContext::get_icon(::graphics::windows::PlatformStyle platform) const {
    auto it = icons_.find(platform);
    if (it != icons_.end()) return it->second;
    return Icon{};
}

void ApplicationContext::set_menu_bar(const ::graphics::components::MenuBarModel &menu_bar) {
    menu_bar_ = menu_bar;
}

const ::graphics::components::MenuBarModel &ApplicationContext::menu_bar() const {
    return menu_bar_;
}

void ApplicationContext::add_window(::graphics::windows::WindowSimulator &&win) {
    windows_.emplace_back(std::move(win));
}

const std::vector<::graphics::windows::WindowSimulator> &ApplicationContext::windows() const {
    return windows_;
}

} // namespace application_context
