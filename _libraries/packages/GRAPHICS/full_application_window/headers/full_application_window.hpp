#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_FULL_APPLICATION_WINDOW_HPP
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_FULL_APPLICATION_WINDOW_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "../../graphics_object.hpp"
#include "components.hpp"
#include "windows.hpp"

namespace graphics {
namespace full_application_window {

enum class Backend {
    Win32,
    Cocoa,
    X11,
    Headless
};

struct WindowConfig : public ::graphics::GraphicsObject {
    std::string title = "CoolBox Window";
    std::size_t width = 960;
    std::size_t height = 640;
    bool visible = true;
    bool resizable = true;

    WindowConfig() = default;
    WindowConfig(std::string window_title,
                 std::size_t window_width,
                 std::size_t window_height,
                 bool window_visible = true,
                 bool window_resizable = true)
        : title(std::move(window_title))
        , width(window_width)
        , height(window_height)
        , visible(window_visible)
        , resizable(window_resizable) {}

    std::string graphics_object_kind() const override { return "windowConfig"; }
    std::string graphics_object_name() const override { return title; }
};

struct RenderEvent : public ::graphics::GraphicsObject {
    Backend backend = Backend::Headless;
    std::uintptr_t native_handle = 0;
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t frame_index = 0;

    std::string graphics_object_kind() const override { return "renderEvent"; }
    std::string graphics_object_name() const override { return "frame-" + std::to_string(frame_index); }
};

struct RenderHooks {
    std::function<void(const RenderEvent&)> on_create;
    std::function<void(const RenderEvent&)> on_render;
    std::function<void(const RenderEvent&)> on_resize;
    std::function<void(const RenderEvent&)> on_tick;
    std::function<void(const RenderEvent&)> on_close;
};

class FullApplicationWindow : public ::graphics::GraphicsObject {
public:
    explicit FullApplicationWindow(WindowConfig config = {});
    ~FullApplicationWindow();

    bool create();
    void show();
    void close();
    bool pump_events();

    bool is_open() const;
    Backend backend() const;
    std::string backend_name() const;
    std::uintptr_t native_handle() const;

    const WindowConfig& config() const;
    void set_title(const std::string& title);

    void set_menu_bar(const ::graphics::components::MenuBarModel& menu_bar);
    const ::graphics::components::MenuBarModel& menu_bar() const;

    void set_platform_style(::graphics::windows::PlatformStyle platform_style);
    ::graphics::windows::PlatformStyle platform_style() const;

    void set_render_hooks(RenderHooks hooks);
    const RenderHooks& render_hooks() const;
    void request_redraw();

    std::string graphics_object_kind() const override { return "fullApplicationWindow"; }
    std::string graphics_object_name() const override { return config().title; }

private:
    struct Impl;
    Impl* impl_;
};

std::string backend_name(Backend backend);
Backend native_backend();

} // namespace full_application_window
} // namespace graphics

#endif  // COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_FULL_APPLICATION_WINDOW_HPP