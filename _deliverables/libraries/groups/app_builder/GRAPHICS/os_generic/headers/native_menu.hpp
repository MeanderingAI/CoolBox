#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace graphics {
namespace full_application_window { class FullApplicationWindow; }
namespace os_generic {

struct MenuAction {
    std::string id;
    std::string label;
    std::string shortcut;
    std::function<void()> on_select;
    bool enabled = true;
    bool checked = false;
    bool separator = false;

    static MenuAction divider();
};

struct Menu {
    std::string title;
    std::vector<MenuAction> items;
};

// The window must outlive this controller. Replacing its low-level menu
// model/handler directly supersedes this controller's installed menu.
class NativeMenuBar {
public:
    explicit NativeMenuBar(full_application_window::FullApplicationWindow& window);
    NativeMenuBar(const NativeMenuBar&) = delete;
    NativeMenuBar& operator=(const NativeMenuBar&) = delete;

    bool supported() const;
    void set_menus(std::vector<Menu> menus);
    void set_enabled(const std::string& command_id, bool enabled);
    void set_checked(const std::string& command_id, bool checked);
    bool enabled(const std::string& command_id) const;
    bool checked(const std::string& command_id) const;

private:
    struct State;
    full_application_window::FullApplicationWindow& window_;
    std::shared_ptr<State> state_;
    void publish();
};

} // namespace os_generic
} // namespace graphics
