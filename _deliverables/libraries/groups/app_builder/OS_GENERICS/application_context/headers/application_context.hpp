#ifndef COOLBOX_APP_BUILDER_OS_GENERICS_APPLICATION_CONTEXT_HPP
#define COOLBOX_APP_BUILDER_OS_GENERICS_APPLICATION_CONTEXT_HPP

#include <map>
#include <string>
#include <vector>

#include "windows.hpp"
#include "components.hpp"

namespace app_builder {
namespace os_generics {

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

    void set_icon(::graphics::windows::PlatformStyle platform, const Icon& icon);
    bool has_icon(::graphics::windows::PlatformStyle platform) const;
    Icon get_icon(::graphics::windows::PlatformStyle platform) const;

    void set_menu_bar(const ::graphics::components::MenuBarModel& menu_bar);
    const ::graphics::components::MenuBarModel& menu_bar() const;

    void add_window(::graphics::windows::WindowSimulator&& win);
    const std::vector<::graphics::windows::WindowSimulator>& windows() const;

private:
    std::string name_;
    std::map<::graphics::windows::PlatformStyle, Icon> icons_;
    ::graphics::components::MenuBarModel menu_bar_;
    std::vector<::graphics::windows::WindowSimulator> windows_;
};

} // namespace os_generics
} // namespace app_builder

#endif
