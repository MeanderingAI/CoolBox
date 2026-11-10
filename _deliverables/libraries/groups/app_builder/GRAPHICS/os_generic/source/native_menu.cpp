#include "native_menu.hpp"
#include "full_application_window.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace graphics::os_generic {

struct NativeMenuBar::State {
    std::vector<Menu> menus;

    MenuAction& action(const std::string& id) {
        for (auto& menu : menus) for (auto& item : menu.items)
            if (!item.separator && item.id == id) return item;
        throw std::invalid_argument("Unknown native menu command: " + id);
    }
};

MenuAction MenuAction::divider() {
    MenuAction item;
    item.separator = true;
    return item;
}

NativeMenuBar::NativeMenuBar(full_application_window::FullApplicationWindow& window)
    : window_(window), state_(std::make_shared<State>()) {}

bool NativeMenuBar::supported() const { return window_.has_native_menus(); }

void NativeMenuBar::set_menus(std::vector<Menu> menus) {
    if (!supported()) throw std::runtime_error("Native menus are unavailable for this window backend.");
    std::unordered_set<std::string> ids;
    for (const auto& menu : menus) {
        if (menu.title.empty()) throw std::invalid_argument("Native menu titles must not be empty.");
        for (const auto& item : menu.items) {
            if (item.separator) continue;
            if (item.id.empty() || item.label.empty() || !item.on_select)
                throw std::invalid_argument("Native menu actions require an ID, label and callback.");
            if (!ids.insert(item.id).second)
                throw std::invalid_argument("Duplicate native menu command: " + item.id);
        }
    }
    // A fresh snapshot keeps an in-flight callback safe if it rebuilds the menu.
    auto state = std::make_shared<State>();
    state->menus = std::move(menus);
    state_ = std::move(state);
    publish();
}

void NativeMenuBar::publish() {
    components::MenuBarModel model;
    for (const auto& definition : state_->menus) {
        components::MenuModel menu(definition.title);
        for (const auto& item : definition.items)
            menu.add_item(item.separator ? components::MenuItem::divider() :
                components::MenuItem::action(item.label, item.shortcut, item.enabled, item.checked));
        model.add_menu(menu);
    }
    const auto state = state_;
    window_.set_menu_command_handler([state](std::size_t menu, std::size_t item, const auto&, const auto&) {
        if (menu >= state->menus.size() || item >= state->menus[menu].items.size()) return;
        const auto& action = state->menus[menu].items[item];
        if (action.enabled && !action.separator) {
            const auto callback = action.on_select;
            callback();
        }
    });
    window_.set_menu_bar(model);
}

void NativeMenuBar::set_enabled(const std::string& id, bool value) {
    state_->action(id).enabled = value;
    publish();
}

void NativeMenuBar::set_checked(const std::string& id, bool value) {
    state_->action(id).checked = value;
    publish();
}

bool NativeMenuBar::enabled(const std::string& id) const { return state_->action(id).enabled; }
bool NativeMenuBar::checked(const std::string& id) const { return state_->action(id).checked; }

} // namespace graphics::os_generic
