#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_FULL_APPLICATION_WINDOW_HPP
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_FULL_APPLICATION_WINDOW_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "../../../GRAPHICS/graphics_object.hpp"
#include "../../../GRAPHICS/charts/headers/graphics.h"
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
    // Minimum size the window manager should allow the user to resize down
    // to (0 = no constraint). Currently enforced on the X11 backend via
    // WM_NORMAL_HINTS; other backends ignore it for now.
    std::size_t min_width = 0;
    std::size_t min_height = 0;

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

struct PointerState {
    int x = 0;
    int y = 0;
    int client_width = 0;
    int client_height = 0;
    bool inside = false;
    bool left_button_down = false;
    bool right_button_down = false;
};

using MenuCommandHandler = std::function<void(std::size_t menu_index,
                                              std::size_t item_index,
                                              const ::graphics::components::MenuModel& menu,
                                              const ::graphics::components::MenuItem& item)>;

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
    void set_menu_command_handler(MenuCommandHandler handler);
    const MenuCommandHandler& menu_command_handler() const;
    bool set_menu_item_checked(std::size_t menu_index, std::size_t item_index, bool checked);
    bool menu_item_checked(std::size_t menu_index, std::size_t item_index) const;

    void set_platform_style(::graphics::windows::PlatformStyle platform_style);
    ::graphics::windows::PlatformStyle platform_style() const;

    void set_render_hooks(RenderHooks hooks);
    const RenderHooks& render_hooks() const;
    void request_redraw();
    bool query_pointer_state(PointerState& state) const;
    void show_info_dialog(const std::string& title, const std::string& message) const;
    bool client_size(int& width, int& height) const;
    void clear_background(unsigned char r, unsigned char g, unsigned char b) const;
    void fill_rect(int left, int top, int right, int bottom,
                   unsigned char r, unsigned char g, unsigned char b) const;
    void draw_arc(int cx, int cy, int radius,
                  double start_angle_deg, double end_angle_deg,
                  unsigned char r, unsigned char g, unsigned char b,
                  int thickness = 1) const;
    void draw_rounded_rect(int left, int top, int right, int bottom,
                           int radius,
                           unsigned char r, unsigned char g, unsigned char b,
                           bool filled = false) const;
    void draw_text_line(int x, int y, const std::string& text,
                        unsigned char r = 235, unsigned char g = 240, unsigned char b = 248) const;
    bool present_canvas(const ::graphics::Canvas& canvas) const;

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