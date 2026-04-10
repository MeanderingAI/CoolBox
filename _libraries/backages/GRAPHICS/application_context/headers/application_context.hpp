#ifndef COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_APPLICATION_CONTEXT_HEADERS_APPLICATION_CONTEXT_HPP
#define COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_APPLICATION_CONTEXT_HEADERS_APPLICATION_CONTEXT_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "windows.hpp"
#include "components.hpp"

namespace application_context {

struct Icon {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::vector<unsigned char> pixels;
};

class ApplicationContext {
public:
    explicit ApplicationContext(std::string name = "AppContext");
    ~ApplicationContext();

    // Icon management per platform
    void set_icon(::graphics::windows::PlatformStyle platform, const Icon &icon);
    bool has_icon(::graphics::windows::PlatformStyle platform) const;
    Icon get_icon(::graphics::windows::PlatformStyle platform) const;

    // Menu bar management (shared for app)
    void set_menu_bar(const ::graphics::components::MenuBarModel &menu_bar);
    const ::graphics::components::MenuBarModel &menu_bar() const;

    // Window registration
    void add_window(::graphics::windows::WindowSimulator &&win);
    const std::vector<::graphics::windows::WindowSimulator> &windows() const;

private:
    std::string name_;
    std::map<::graphics::windows::PlatformStyle, Icon> icons_;
    ::graphics::components::MenuBarModel menu_bar_;
    std::vector<::graphics::windows::WindowSimulator> windows_;
};

} // namespace application_context
#endif  // COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_APPLICATION_CONTEXT_HEADERS_APPLICATION_CONTEXT_HPP
