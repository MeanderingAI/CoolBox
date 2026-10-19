#include "full_application_window.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <thread>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/GL.h>
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include "cocoa_native_menu.hpp"
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if defined(GRAPHICS_HAVE_GTK3)
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif
#endif

namespace graphics {
namespace full_application_window {

namespace {

Backend detect_backend() {
#if defined(_WIN32)
    return Backend::Win32;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    return Backend::Cocoa;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    return Backend::X11;
#else
    return Backend::Headless;
#endif
}

} // namespace

struct FullApplicationWindow::Impl {
    WindowConfig config;
    Backend backend = detect_backend();
    bool open = false;
    std::size_t frame_index = 0;
    ::graphics::components::MenuBarModel menu_bar_model;
    MenuCommandHandler menu_command_handler;
    RenderHooks hooks;
    ::graphics::windows::PlatformStyle platform =
#if defined(_WIN32)
        ::graphics::windows::PlatformStyle::Windows;
#elif defined(__APPLE__)
        ::graphics::windows::PlatformStyle::MacOS;
#else
        ::graphics::windows::PlatformStyle::Linux;
#endif

#if defined(_WIN32)
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
    HMENU native_menu_bar = nullptr;
    std::unordered_map<UINT, std::pair<std::size_t, std::size_t>> command_to_menu_item;
    UINT next_menu_command_id = 40000U;
    mutable bool gl_renderer_attempted = false;
    mutable bool gl_renderer_ready = false;
    mutable HDC gl_hdc = nullptr;
    mutable HGLRC gl_context = nullptr;
    mutable GLuint gl_texture = 0U;
    mutable HDC ui_backbuffer_dc = nullptr;
    mutable HBITMAP ui_backbuffer_bitmap = nullptr;
    mutable HBITMAP ui_backbuffer_old_bitmap = nullptr;
    mutable int ui_backbuffer_width = 0;
    mutable int ui_backbuffer_height = 0;
    mutable bool ui_backbuffer_used_this_frame = false;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    void* application = nullptr;
    void* window = nullptr;
    void* cocoa_menu = nullptr;
    bool mac_control_click = false;

    // Software pixel backbuffer: RGBA 8-bit packed rows.
    mutable std::vector<unsigned char> mac_pixels;
    mutable int mac_buf_w = 0;
    mutable int mac_buf_h = 0;

    bool mac_ensure_buf(int w, int h) const {
        if (w <= 0 || h <= 0) return false;
        if (w == mac_buf_w && h == mac_buf_h) return true;
        mac_buf_w = w;
        mac_buf_h = h;
        mac_pixels.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
        return true;
    }

    void mac_fill_rect(int x0, int y0, int x1, int y1,
                       unsigned char r, unsigned char g, unsigned char b) const {
        if (!mac_ensure_buf(mac_buf_w, mac_buf_h)) return;
        x0 = std::max(0, x0); y0 = std::max(0, y0);
        x1 = std::min(mac_buf_w, x1); y1 = std::min(mac_buf_h, y1);
        for (int y = y0; y < y1; ++y) {
            unsigned char* row = mac_pixels.data() + (y * mac_buf_w + x0) * 4;
            for (int x = x0; x < x1; ++x, row += 4) {
                row[0] = r; row[1] = g; row[2] = b; row[3] = 255;
            }
        }
    }

    // Minimal 5x7 ASCII bitmap font (printable chars 32-126).
    static const unsigned char k_font_5x7[95][7];

    void mac_draw_text(int x, int y,
                       const std::string& text,
                       unsigned char r, unsigned char g, unsigned char b) const {
        if (!mac_ensure_buf(mac_buf_w, mac_buf_h)) return;
        int cx = x;
        for (unsigned char ch : text) {
            if (ch < 32 || ch > 126) { cx += 6; continue; }
            const unsigned char* glyph = k_font_5x7[ch - 32];
            for (int row = 0; row < 7; ++row) {
                const int py = y + row;
                if (py < 0 || py >= mac_buf_h) continue;
                for (int col = 0; col < 5; ++col) {
                    const int px = cx + col;
                    if (px < 0 || px >= mac_buf_w) continue;
                    if (glyph[row] & (0x10u >> col)) {
                        unsigned char* p = mac_pixels.data() + (py * mac_buf_w + px) * 4;
                        p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
                    }
                }
            }
            cx += 6;
        }
    }

    void mac_blit_to_window() const {
        if (!window || mac_pixels.empty() || mac_buf_w <= 0 || mac_buf_h <= 0) return;

        using IdFn   = id   (*)(id, SEL);
        using VoidFn = void (*)(id, SEL);

        // Build a CGImage from our RGBA pixel buffer.
        CGColorSpaceRef csp = CGColorSpaceCreateDeviceRGB();
        CGDataProviderRef provider = CGDataProviderCreateWithData(
            nullptr,
            mac_pixels.data(),
            mac_pixels.size(),
            nullptr);
        CGImageRef img = CGImageCreate(
            static_cast<std::size_t>(mac_buf_w),
            static_cast<std::size_t>(mac_buf_h),
            8, 32,
            static_cast<std::size_t>(mac_buf_w) * 4,
            csp,
            kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast,
            provider,
            nullptr, false, kCGRenderingIntentDefault);
        CGColorSpaceRelease(csp);
        CGDataProviderRelease(provider);
        if (!img) return;

        // Get the contentView and ensure it is layer-backed.
        auto* content_view = reinterpret_cast<IdFn>(objc_msgSend)(
            reinterpret_cast<id>(window), sel_registerName("contentView"));
        if (!content_view) { CGImageRelease(img); return; }

        reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(
            content_view, sel_registerName("setWantsLayer:"), YES);

        auto* layer = reinterpret_cast<IdFn>(objc_msgSend)(
            content_view, sel_registerName("layer"));
        if (!layer) { CGImageRelease(img); return; }

        // Disable the default implicit CALayer animation on content changes.
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(
            layer, sel_registerName("setActions:"),
            reinterpret_cast<id>(objc_getClass("NSDictionary")));

        // contentsGravity = "resize" stretches the image to fill the entire layer.
        auto* gravity_str = reinterpret_cast<id (*)(id, SEL, const char*)>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSString")),
            sel_registerName("stringWithUTF8String:"),
            "resize");
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(
            layer, sel_registerName("setContentsGravity:"), gravity_str);

        // contentsScale = 1.0 so our pixel buffer maps 1:1 to layer points
        // (prevents Retina halving the apparent size).
        reinterpret_cast<void (*)(id, SEL, double)>(objc_msgSend)(
            layer, sel_registerName("setContentsScale:"), 1.0);

        // Set the CGImage as the layer content directly — fastest path.
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(
            layer, sel_registerName("setContents:"),
            reinterpret_cast<id>(img));

        reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(
            layer, sel_registerName("setNeedsDisplayOnBoundsChange:"), YES);

        // Trigger an immediate redisplay.
        reinterpret_cast<VoidFn>(objc_msgSend)(
            reinterpret_cast<id>(window), sel_registerName("display"));

        CGImageRelease(img);
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    Display* display = nullptr;
    std::unordered_set<unsigned int> held_keys;
    ::Window window = 0;
    Atom delete_message = 0;
#if defined(GRAPHICS_HAVE_GTK3)
    GtkWidget* gtk_window = nullptr;
    GtkWidget* gtk_box = nullptr;
    GtkWidget* gtk_canvas = nullptr;
    GtkWidget* gtk_menu = nullptr;
    GtkAccelGroup* gtk_accels = nullptr;

    void rebuild_native_menu() {
        if (!gtk_window) return;
        if (gtk_menu) {
            gtk_widget_destroy(gtk_menu);
            gtk_menu = nullptr;
        }
        if (gtk_accels) {
            gtk_window_remove_accel_group(GTK_WINDOW(gtk_window), gtk_accels);
            g_object_unref(gtk_accels);
            gtk_accels = nullptr;
        }
        if (menu_bar_model.menus.empty()) return;
        gtk_accels = gtk_accel_group_new();
        gtk_window_add_accel_group(GTK_WINDOW(gtk_window), gtk_accels);
        gtk_menu = gtk_menu_bar_new();
        gtk_box_pack_start(GTK_BOX(gtk_box), gtk_menu, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(gtk_box), gtk_menu, 0);
        struct Command { Impl* impl; std::size_t menu; std::size_t item; };
        for (std::size_t m = 0; m < menu_bar_model.menus.size(); ++m) {
            const auto& menu = menu_bar_model.menus[m];
            GtkWidget* top = gtk_menu_item_new_with_label(menu.title.c_str());
            GtkWidget* popup = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(top), popup);
            gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu), top);
            for (std::size_t i = 0; i < menu.items.size(); ++i) {
                const auto& item = menu.items[i];
                GtkWidget* native = item.separator ? gtk_separator_menu_item_new()
                    : gtk_check_menu_item_new_with_label(item.label.c_str());
                gtk_menu_shell_append(GTK_MENU_SHELL(popup), native);
                if (item.separator) continue;
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(native), item.checked);
                gtk_widget_set_sensitive(native, item.enabled);
                std::string key;
                GdkModifierType modifiers = static_cast<GdkModifierType>(0);
                std::size_t start = 0, plus;
                while ((plus = item.shortcut.find('+', start)) != std::string::npos) {
                    std::string token = item.shortcut.substr(start, plus - start);
                    std::transform(token.begin(), token.end(), token.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (token == "ctrl" || token == "control" || token == "cmd")
                        modifiers = static_cast<GdkModifierType>(modifiers | GDK_CONTROL_MASK);
                    if (token == "shift") modifiers = static_cast<GdkModifierType>(modifiers | GDK_SHIFT_MASK);
                    if (token == "alt") modifiers = static_cast<GdkModifierType>(modifiers | GDK_MOD1_MASK);
                    start = plus + 1;
                }
                key = item.shortcut.substr(start);
                if (key == "Space") key = "space";
                if (key == "Enter") key = "Return";
                guint keyval = key.empty() ? 0 : gdk_keyval_from_name(key.c_str());
                if (keyval != 0 && keyval != GDK_KEY_VoidSymbol)
                    gtk_widget_add_accelerator(native, "activate", gtk_accels,
                        gdk_keyval_to_lower(keyval), modifiers, GTK_ACCEL_VISIBLE);
                g_signal_connect_data(native, "activate", reinterpret_cast<GCallback>(+[](GtkMenuItem* widget, gpointer data) {
                    const auto command = *static_cast<Command*>(data);
                    g_object_ref(widget);
                    command.impl->dispatch_menu(command.menu, command.item);
                    const auto& menus = command.impl->menu_bar_model.menus;
                    if (!gtk_widget_in_destruction(GTK_WIDGET(widget)) &&
                        command.menu < menus.size() && command.item < menus[command.menu].items.size()) {
                        // Restoring a non-toggle command's check state also emits "activate".
                        g_signal_handlers_block_matched(widget, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, data);
                        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
                            menus[command.menu].items[command.item].checked);
                        g_signal_handlers_unblock_matched(widget, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, data);
                    }
                    g_object_unref(widget);
                }), new Command{this, m, i}, +[](gpointer data, GClosure*) {
                    delete static_cast<Command*>(data);
                }, static_cast<GConnectFlags>(0));
            }
        }
        gtk_widget_show_all(gtk_menu);
    }

    bool create_gtk_window() {
        // Our rendering API exposes an XID, so deliberately require the X11 GDK backend.
        gdk_set_allowed_backends("x11");
        if (!gtk_init_check(nullptr, nullptr) ||
            !GDK_IS_X11_DISPLAY(gdk_display_get_default())) return false;
        gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(gtk_window), config.title.c_str());
        gtk_window_set_resizable(GTK_WINDOW(gtk_window), config.resizable);
        gtk_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(gtk_window), gtk_box);
        gtk_canvas = gtk_drawing_area_new();
        gtk_widget_set_can_focus(gtk_canvas, TRUE);
        gtk_widget_set_app_paintable(gtk_canvas, TRUE);
        // Keep the existing Xlib drawing path on the drawing area's real X window.
        gtk_widget_set_double_buffered(gtk_canvas, FALSE);
        gtk_widget_set_size_request(gtk_canvas, static_cast<int>(config.min_width),
                                    static_cast<int>(config.min_height));
        gtk_box_pack_end(GTK_BOX(gtk_box), gtk_canvas, TRUE, TRUE, 0);
        gtk_widget_add_events(gtk_canvas, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
            GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
        g_signal_connect(gtk_canvas, "draw", reinterpret_cast<GCallback>(+[](GtkWidget*, cairo_t* cr, gpointer data) -> gboolean {
            auto* self = static_cast<Impl*>(data);
            cairo_surface_flush(cairo_get_target(cr));
            if (self->open && self->hooks.on_render)
                self->hooks.on_render(self->make_render_event(self->window, self->frame_index++));
            if (self->display) XFlush(self->display);
            return TRUE;
        }), this);
        g_signal_connect(gtk_canvas, "size-allocate", reinterpret_cast<GCallback>(+[](GtkWidget*, GtkAllocation* size, gpointer data) {
            auto* self = static_cast<Impl*>(data);
            self->config.width = static_cast<std::size_t>(std::max(1, size->width));
            self->config.height = static_cast<std::size_t>(std::max(1, size->height));
            if (self->open && self->hooks.on_resize)
                self->hooks.on_resize(self->make_render_event(self->window, self->frame_index));
        }), this);
        const auto pointer = +[](GtkWidget* widget, GdkEvent* event, gpointer data) -> gboolean {
            auto* self = static_cast<Impl*>(data);
            double x = 0, y = 0;
            gdk_event_get_coords(event, &x, &y);
            guint button = 0;
            gdk_event_get_button(event, &button);
            if (event->type != GDK_MOTION_NOTIFY && button != 1 && button != 3) return FALSE;
            PointerPressEvent pointer{static_cast<int>(x), static_cast<int>(y),
                button == 3 ? PointerButton::Right : PointerButton::Left};
            if (event->type == GDK_BUTTON_PRESS) {
                gtk_widget_grab_focus(widget);
                if (button == 1) {
                    gtk_grab_add(widget);
                    gdk_seat_grab(gdk_display_get_default_seat(gtk_widget_get_display(widget)),
                        gtk_widget_get_window(widget), GDK_SEAT_CAPABILITY_POINTER, FALSE,
                        nullptr, event, nullptr, nullptr);
                }
                if (self->hooks.on_pointer_press) self->hooks.on_pointer_press(pointer);
            } else if (event->type == GDK_BUTTON_RELEASE) {
                if (button == 1) {
                    gdk_seat_ungrab(gdk_display_get_default_seat(gtk_widget_get_display(widget)));
                    if (gtk_widget_has_grab(widget)) gtk_grab_remove(widget);
                }
                if (self->hooks.on_pointer_release) self->hooks.on_pointer_release(pointer);
            } else if (self->hooks.on_pointer_move) {
                GdkModifierType state{};
                gdk_event_get_state(event, &state);
                pointer.button = (state & GDK_BUTTON3_MASK) ? PointerButton::Right : PointerButton::Left;
                self->hooks.on_pointer_move(pointer);
            }
            return TRUE;
        };
        g_signal_connect(gtk_canvas, "button-press-event", G_CALLBACK(pointer), this);
        g_signal_connect(gtk_canvas, "button-release-event", G_CALLBACK(pointer), this);
        g_signal_connect(gtk_canvas, "motion-notify-event", G_CALLBACK(pointer), this);
        g_signal_connect(gtk_canvas, "scroll-event", reinterpret_cast<GCallback>(+[](GtkWidget*, GdkEventScroll* event, gpointer data) -> gboolean {
            auto* self = static_cast<Impl*>(data);
            double delta = event->direction == GDK_SCROLL_UP ? 1.0 :
                event->direction == GDK_SCROLL_DOWN ? -1.0 : -event->delta_y;
            if (self->hooks.on_scroll)
                self->hooks.on_scroll(ScrollEvent{static_cast<int>(event->x), static_cast<int>(event->y),
                    delta, (event->state & GDK_SHIFT_MASK) != 0});
            return TRUE;
        }), this);
        g_signal_connect(gtk_window, "key-press-event", reinterpret_cast<GCallback>(+[](GtkWidget* widget, GdkEventKey* event, gpointer data) -> gboolean {
            auto* self = static_cast<Impl*>(data);
            if (gtk_accel_groups_activate(G_OBJECT(widget), event->keyval,
                    static_cast<GdkModifierType>(event->state & gtk_accelerator_get_default_mod_mask())))
                return TRUE;
            const bool repeat = !self->held_keys.insert(event->hardware_keycode).second;
            int key = static_cast<int>(gdk_keyval_to_upper(event->keyval));
            switch (event->keyval) {
                case GDK_KEY_Left: key = static_cast<int>(Key::Left); break;
                case GDK_KEY_Right: key = static_cast<int>(Key::Right); break;
                case GDK_KEY_Home: key = static_cast<int>(Key::Home); break;
                case GDK_KEY_End: key = static_cast<int>(Key::End); break;
                case GDK_KEY_Delete: key = static_cast<int>(Key::Delete); break;
                case GDK_KEY_BackSpace: key = static_cast<int>(Key::Backspace); break;
                case GDK_KEY_Return: key = static_cast<int>(Key::Enter); break;
                case GDK_KEY_Escape: key = static_cast<int>(Key::Escape); break;
                case GDK_KEY_Tab:
                case GDK_KEY_ISO_Left_Tab: key = 9; break;
                default: break;
            }
            if (self->hooks.on_key) self->hooks.on_key(KeyEvent{key,
                (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0, repeat});
            if (self->hooks.on_text_input && !(event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
                const gunichar character = gdk_keyval_to_unicode(event->keyval);
                if (character >= 32 && character != 127) {
                    char utf8[7]{};
                    const int length = g_unichar_to_utf8(character, utf8);
                    self->hooks.on_text_input(std::string(utf8, length));
                }
            }
            return self->hooks.on_key ? TRUE : FALSE;
        }), this);
        g_signal_connect(gtk_window, "key-release-event", reinterpret_cast<GCallback>(+[](GtkWidget*, GdkEventKey* event, gpointer data) -> gboolean {
            static_cast<Impl*>(data)->held_keys.erase(event->hardware_keycode);
            return FALSE;
        }), this);
        g_signal_connect(gtk_window, "focus-out-event", reinterpret_cast<GCallback>(+[](GtkWidget*, GdkEventFocus*, gpointer data) -> gboolean {
            static_cast<Impl*>(data)->held_keys.clear();
            return FALSE;
        }), this);
        g_signal_connect(gtk_window, "delete-event", reinterpret_cast<GCallback>(+[](GtkWidget*, GdkEvent*, gpointer data) -> gboolean {
            auto* self = static_cast<Impl*>(data);
            if (self->hooks.on_close) self->hooks.on_close(self->make_render_event(self->window, self->frame_index));
            return FALSE;
        }), this);
        g_signal_connect(gtk_window, "destroy", reinterpret_cast<GCallback>(+[](GtkWidget*, gpointer data) {
            auto* self = static_cast<Impl*>(data);
            self->open = false;
            self->window = 0;
            self->gtk_window = self->gtk_box = self->gtk_canvas = self->gtk_menu = nullptr;
        }), this);
        rebuild_native_menu();
        gtk_widget_show_all(gtk_box);
        GtkRequisition minimum{}, natural{};
        gtk_widget_get_preferred_size(gtk_menu ? gtk_menu : gtk_canvas, &minimum, &natural);
        const int menu_height = gtk_menu ? natural.height : 0;
        gtk_window_set_default_size(GTK_WINDOW(gtk_window), static_cast<int>(config.width),
                                    static_cast<int>(config.height) + menu_height);
        gtk_widget_realize(gtk_window);
        gtk_widget_realize(gtk_canvas);
        auto* drawable = gtk_widget_get_window(gtk_canvas);
        display = GDK_WINDOW_XDISPLAY(drawable);
        window = GDK_WINDOW_XID(drawable);
        open = true;
        if (config.visible) gtk_widget_show(gtk_window);
        gtk_widget_grab_focus(gtk_canvas);
        return true;
    }
#endif
#endif

    void dispatch_menu(std::size_t menu_index, std::size_t item_index) {
        if (menu_index >= menu_bar_model.menus.size()) return;
        const auto menu = menu_bar_model.menus[menu_index];
        if (item_index >= menu.items.size()) return;
        const auto item = menu.items[item_index];
        const auto handler = menu_command_handler;
        if (item.enabled && !item.separator && handler) handler(menu_index, item_index, menu, item);
    }

#if defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    void rebuild_native_menu() {
        detail::destroy_cocoa_menu(cocoa_menu);
        cocoa_menu = nullptr;
        if (window && !menu_bar_model.menus.empty())
            cocoa_menu = detail::create_cocoa_menu(window, menu_bar_model,
                [this](std::size_t menu, std::size_t item) { dispatch_menu(menu, item); });
    }
#endif

    RenderEvent make_render_event(std::uintptr_t native_handle = 0U,
                                  std::size_t event_frame_index = 0U) const {
        RenderEvent event{};
        event.backend = backend;
        event.native_handle = native_handle;
        event.width = config.width;
        event.height = config.height;
        event.frame_index = event_frame_index;
        return event;
    }

#if defined(_WIN32)
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        Impl* impl = nullptr;
        if (message == WM_NCCREATE) {
            auto* create_struct = reinterpret_cast<CREATESTRUCTA*>(lparam);
            impl = static_cast<Impl*>(create_struct->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
        } else {
            impl = reinterpret_cast<Impl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (impl && message == WM_GETMINMAXINFO &&
            (impl->config.min_width > 0 || impl->config.min_height > 0)) {
            RECT bounds{0, 0, static_cast<LONG>(impl->config.min_width),
                        static_cast<LONG>(impl->config.min_height)};
            AdjustWindowRectEx(&bounds, static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE)),
                               GetMenu(hwnd) != nullptr,
                               static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_EXSTYLE)));
            auto* sizes = reinterpret_cast<MINMAXINFO*>(lparam);
            if (impl->config.min_width > 0) sizes->ptMinTrackSize.x = bounds.right - bounds.left;
            if (impl->config.min_height > 0) sizes->ptMinTrackSize.y = bounds.bottom - bounds.top;
            return 0;
        }

        if (impl && message == WM_CHAR && impl->hooks.on_text_input) {
            static thread_local wchar_t high_surrogate = 0;
            const wchar_t character = static_cast<wchar_t>(wparam);
            if (character >= 0xd800 && character <= 0xdbff) {
                high_surrogate = character;
                return 0;
            }
            wchar_t characters[2]{character, 0};
            int count = 1;
            if (character >= 0xdc00 && character <= 0xdfff && high_surrogate) {
                characters[0] = high_surrogate;
                characters[1] = character;
                count = 2;
            }
            high_surrogate = 0;
            if (character >= 32 && character != 127) {
                char utf8[8]{};
                const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                    characters, count, utf8, sizeof(utf8), nullptr, nullptr);
                if (length > 0) impl->hooks.on_text_input(std::string(utf8, length));
            }
            return 0;
        }
        if (impl && message == WM_KEYDOWN && impl->hooks.on_key) {
            int key = static_cast<int>(wparam);
            switch (wparam) {
                case VK_LEFT: key = static_cast<int>(Key::Left); break;
                case VK_RIGHT: key = static_cast<int>(Key::Right); break;
                case VK_HOME: key = static_cast<int>(Key::Home); break;
                case VK_END: key = static_cast<int>(Key::End); break;
                case VK_DELETE: key = static_cast<int>(Key::Delete); break;
                default: break;
            }
            impl->hooks.on_key(KeyEvent{key, (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                (GetKeyState(VK_SHIFT) & 0x8000) != 0, (lparam & (1LL << 30)) != 0});
            return 0;
        }

        if (impl && (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                     message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MOUSEMOVE)) {
            const PointerPressEvent event{
                static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam)),
                (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP ||
                 (message == WM_MOUSEMOVE && (wparam & MK_RBUTTON))) ? PointerButton::Right : PointerButton::Left};
            if (message == WM_LBUTTONDOWN) SetCapture(hwnd);
            if (message == WM_LBUTTONUP && GetCapture() == hwnd) ReleaseCapture();
            if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) {
                if (impl->hooks.on_pointer_press) impl->hooks.on_pointer_press(event);
            } else if (message == WM_LBUTTONUP || message == WM_RBUTTONUP) {
                if (impl->hooks.on_pointer_release) impl->hooks.on_pointer_release(event);
            } else if (impl->hooks.on_pointer_move) impl->hooks.on_pointer_move(event);
            return 0;
        }

        if (impl && message == WM_SIZE) {
            impl->config.width = static_cast<std::size_t>(LOWORD(lparam));
            impl->config.height = static_cast<std::size_t>(HIWORD(lparam));
            impl->release_ui_backbuffer();
            if (impl->hooks.on_resize) {
                impl->hooks.on_resize(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index));
            }
            return 0;
        }

        if (impl && message == WM_MOUSEWHEEL && impl->hooks.on_scroll) {
            POINT point{static_cast<short>(LOWORD(lparam)),
                        static_cast<short>(HIWORD(lparam))};
            ScreenToClient(hwnd, &point);
            impl->hooks.on_scroll(ScrollEvent{
                point.x,
                point.y,
                static_cast<double>(static_cast<short>(HIWORD(wparam))) / WHEEL_DELTA,
                (LOWORD(wparam) & MK_SHIFT) != 0});
            return 0;
        }

        if (impl && message == WM_ERASEBKGND) {
            // Backbuffer path handles full repaint, so skip default background erase.
            return 1;
        }

        if (impl && message == WM_PAINT) {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            impl->ui_backbuffer_used_this_frame = false;
            if (impl->hooks.on_render) {
                impl->hooks.on_render(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index++));
            }
            if (impl->ui_backbuffer_used_this_frame) {
                impl->present_ui_backbuffer(paint.hdc);
            }
            EndPaint(hwnd, &paint);
            return 0;
        }

        if (impl && message == WM_COMMAND && lparam == 0) {
            const UINT command_id = LOWORD(wparam);
            const auto found = impl->command_to_menu_item.find(command_id);
            if (found != impl->command_to_menu_item.end()) {
                const std::size_t menu_index = found->second.first;
                const std::size_t item_index = found->second.second;
                if (menu_index < impl->menu_bar_model.menus.size()) {
                    auto& menu = impl->menu_bar_model.menus[menu_index];
                    if (item_index < menu.items.size()) {
                        const auto& item = menu.items[item_index];
                        if (item.enabled && !item.separator && impl->menu_command_handler) {
                            impl->menu_command_handler(menu_index, item_index, menu, item);
                        }
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }
            }
        }

        if (impl && message == WM_CLOSE) {
            if (impl->hooks.on_close) {
                impl->hooks.on_close(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index));
            }
            DestroyWindow(hwnd);
            return 0;
        }

        if (impl && message == WM_DESTROY) {
            impl->open = false;
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    void clear_native_menu() {
        command_to_menu_item.clear();
        next_menu_command_id = 40000U;
        if (hwnd) {
            SetMenu(hwnd, nullptr);
        }
        if (native_menu_bar) {
            DestroyMenu(native_menu_bar);
            native_menu_bar = nullptr;
        }
    }

    void rebuild_native_menu() {
        clear_native_menu();
        if (!hwnd) {
            return;
        }
        if (menu_bar_model.menus.empty()) {
            DrawMenuBar(hwnd);
            return;
        }

        native_menu_bar = CreateMenu();
        if (!native_menu_bar) {
            return;
        }

        for (std::size_t menu_index = 0; menu_index < menu_bar_model.menus.size(); ++menu_index) {
            const auto& menu = menu_bar_model.menus[menu_index];
            HMENU popup = CreatePopupMenu();
            if (!popup) {
                continue;
            }

            for (std::size_t item_index = 0; item_index < menu.items.size(); ++item_index) {
                const auto& item = menu.items[item_index];
                if (item.separator) {
                    AppendMenuA(popup, MF_SEPARATOR, 0, nullptr);
                    continue;
                }

                const UINT command_id = next_menu_command_id++;
                UINT flags = MF_STRING;
                if (!item.enabled) {
                    flags |= MF_GRAYED;
                }
                if (item.checked) {
                    flags |= MF_CHECKED;
                }
                const auto label = item.shortcut.empty() ? item.label : item.label + "\t" + item.shortcut;
                AppendMenuA(popup, flags, command_id, label.c_str());
                command_to_menu_item[command_id] = {menu_index, item_index};
            }

            AppendMenuA(native_menu_bar,
                        MF_POPUP,
                        reinterpret_cast<UINT_PTR>(popup),
                        menu.title.c_str());
        }

        SetMenu(hwnd, native_menu_bar);
        DrawMenuBar(hwnd);
    }

    void release_gl_resources() const {
        if (gl_context) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(gl_context);
            gl_context = nullptr;
        }
        if (gl_hdc && hwnd) {
            ReleaseDC(hwnd, gl_hdc);
            gl_hdc = nullptr;
        }
        gl_texture = 0U;
        gl_renderer_ready = false;
    }

    void release_ui_backbuffer() const {
        if (ui_backbuffer_dc) {
            if (ui_backbuffer_old_bitmap) {
                SelectObject(ui_backbuffer_dc, ui_backbuffer_old_bitmap);
                ui_backbuffer_old_bitmap = nullptr;
            }
            if (ui_backbuffer_bitmap) {
                DeleteObject(ui_backbuffer_bitmap);
                ui_backbuffer_bitmap = nullptr;
            }
            DeleteDC(ui_backbuffer_dc);
            ui_backbuffer_dc = nullptr;
        }
        ui_backbuffer_width = 0;
        ui_backbuffer_height = 0;
    }

    bool ensure_ui_backbuffer(int width, int height) const {
        if (!hwnd || width <= 0 || height <= 0) {
            return false;
        }
        if (ui_backbuffer_dc && ui_backbuffer_bitmap &&
            ui_backbuffer_width == width && ui_backbuffer_height == height) {
            return true;
        }

        release_ui_backbuffer();

        HDC window_dc = GetDC(hwnd);
        if (!window_dc) {
            return false;
        }

        ui_backbuffer_dc = CreateCompatibleDC(window_dc);
        ui_backbuffer_bitmap = CreateCompatibleBitmap(window_dc, width, height);
        ReleaseDC(hwnd, window_dc);

        if (!ui_backbuffer_dc || !ui_backbuffer_bitmap) {
            release_ui_backbuffer();
            return false;
        }

        ui_backbuffer_old_bitmap = reinterpret_cast<HBITMAP>(SelectObject(ui_backbuffer_dc, ui_backbuffer_bitmap));
        ui_backbuffer_width = width;
        ui_backbuffer_height = height;
        return true;
    }

    void present_ui_backbuffer(HDC target_dc = nullptr) const {
        if (!hwnd || !ui_backbuffer_dc || !ui_backbuffer_bitmap) {
            return;
        }

        RECT rc{};
        GetClientRect(hwnd, &rc);
        const int width = std::max(0, static_cast<int>(rc.right - rc.left));
        const int height = std::max(0, static_cast<int>(rc.bottom - rc.top));
        if (width <= 0 || height <= 0) {
            return;
        }

        HDC dc = target_dc;
        bool acquired = false;
        if (!dc) {
            dc = GetDC(hwnd);
            acquired = dc != nullptr;
        }
        if (!dc) {
            return;
        }

        BitBlt(dc,
               0,
               0,
               std::min(width, ui_backbuffer_width),
               std::min(height, ui_backbuffer_height),
               ui_backbuffer_dc,
               0,
               0,
               SRCCOPY);

        if (acquired) {
            ReleaseDC(hwnd, dc);
        }
    }

    bool ensure_gl_renderer() const {
        if (!hwnd || !IsWindow(hwnd)) {
            return false;
        }
        if (gl_renderer_ready && gl_context && gl_hdc && gl_texture != 0U) {
            return true;
        }
        if (gl_renderer_attempted) {
            return false;
        }

        gl_renderer_attempted = true;
        gl_hdc = GetDC(hwnd);
        if (!gl_hdc) {
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        const int format = ChoosePixelFormat(gl_hdc, &pfd);
        if (format == 0 || !SetPixelFormat(gl_hdc, format, &pfd)) {
            release_gl_resources();
            return false;
        }

        gl_context = wglCreateContext(gl_hdc);
        if (!gl_context || !wglMakeCurrent(gl_hdc, gl_context)) {
            release_gl_resources();
            return false;
        }

        glGenTextures(1, &gl_texture);
        if (gl_texture == 0U) {
            release_gl_resources();
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, gl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        gl_renderer_ready = true;
        return true;
    }
#endif
};

// ─── 5×7 pixel font (printable ASCII 32–126) ──────────────────────────────
// Each entry is 7 rows; each row is a 5-bit mask stored in the low 5 bits
// (bit 4 = leftmost pixel).
#if defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
// clang-format off
const unsigned char FullApplicationWindow::Impl::k_font_5x7[95][7] = {
// ' '
{0x00,0x00,0x00,0x00,0x00,0x00,0x00},
// '!'
{0x04,0x04,0x04,0x04,0x00,0x04,0x00},
// '"'
{0x0a,0x0a,0x00,0x00,0x00,0x00,0x00},
// '#'
{0x0a,0x1f,0x0a,0x0a,0x1f,0x0a,0x00},
// '$'
{0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04},
// '%'
{0x18,0x19,0x02,0x04,0x13,0x03,0x00},
// '&'
{0x0c,0x12,0x14,0x08,0x15,0x12,0x0d},
// '\''
{0x04,0x04,0x00,0x00,0x00,0x00,0x00},
// '('
{0x02,0x04,0x08,0x08,0x08,0x04,0x02},
// ')'
{0x08,0x04,0x02,0x02,0x02,0x04,0x08},
// '*'
{0x00,0x04,0x15,0x0e,0x15,0x04,0x00},
// '+'
{0x00,0x04,0x04,0x1f,0x04,0x04,0x00},
// ','
{0x00,0x00,0x00,0x00,0x06,0x04,0x08},
// '-'
{0x00,0x00,0x00,0x1f,0x00,0x00,0x00},
// '.'
{0x00,0x00,0x00,0x00,0x00,0x06,0x00},
// '/'
{0x01,0x01,0x02,0x04,0x08,0x10,0x10},
// '0'
{0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},
// '1'
{0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
// '2'
{0x0e,0x11,0x01,0x06,0x08,0x10,0x1f},
// '3'
{0x1f,0x02,0x04,0x02,0x01,0x11,0x0e},
// '4'
{0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},
// '5'
{0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e},
// '6'
{0x06,0x08,0x10,0x1e,0x11,0x11,0x0e},
// '7'
{0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
// '8'
{0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},
// '9'
{0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c},
// ':'
{0x00,0x06,0x00,0x00,0x06,0x00,0x00},
// ';'
{0x00,0x06,0x00,0x00,0x06,0x04,0x08},
// '<'
{0x02,0x04,0x08,0x10,0x08,0x04,0x02},
// '='
{0x00,0x1f,0x00,0x00,0x1f,0x00,0x00},
// '>'
{0x08,0x04,0x02,0x01,0x02,0x04,0x08},
// '?'
{0x0e,0x11,0x01,0x06,0x04,0x00,0x04},
// '@'
{0x0e,0x11,0x17,0x15,0x17,0x10,0x0e},
// 'A'
{0x04,0x0a,0x11,0x11,0x1f,0x11,0x11},
// 'B'
{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
// 'C'
{0x0e,0x11,0x10,0x10,0x10,0x11,0x0e},
// 'D'
{0x1c,0x12,0x11,0x11,0x11,0x12,0x1c},
// 'E'
{0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},
// 'F'
{0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
// 'G'
{0x0e,0x11,0x10,0x17,0x11,0x11,0x0f},
// 'H'
{0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
// 'I'
{0x0e,0x04,0x04,0x04,0x04,0x04,0x0e},
// 'J'
{0x07,0x02,0x02,0x02,0x02,0x12,0x0c},
// 'K'
{0x11,0x12,0x14,0x18,0x14,0x12,0x11},
// 'L'
{0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
// 'M'
{0x11,0x1b,0x15,0x15,0x11,0x11,0x11},
// 'N'
{0x11,0x11,0x19,0x15,0x13,0x11,0x11},
// 'O'
{0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},
// 'P'
{0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
// 'Q'
{0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},
// 'R'
{0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
// 'S'
{0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},
// 'T'
{0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
// 'U'
{0x11,0x11,0x11,0x11,0x11,0x11,0x0e},
// 'V'
{0x11,0x11,0x11,0x11,0x0a,0x0a,0x04},
// 'W'
{0x11,0x11,0x15,0x15,0x15,0x0a,0x0a},
// 'X'
{0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
// 'Y'
{0x11,0x11,0x0a,0x04,0x04,0x04,0x04},
// 'Z'
{0x1f,0x01,0x02,0x04,0x08,0x10,0x1f},
// '['
{0x0e,0x08,0x08,0x08,0x08,0x08,0x0e},
// '\\'
{0x10,0x10,0x08,0x04,0x02,0x01,0x01},
// ']'
{0x0e,0x02,0x02,0x02,0x02,0x02,0x0e},
// '^'
{0x04,0x0a,0x11,0x00,0x00,0x00,0x00},
// '_'
{0x00,0x00,0x00,0x00,0x00,0x00,0x1f},
// '`'
{0x08,0x04,0x00,0x00,0x00,0x00,0x00},
// 'a'
{0x00,0x00,0x0e,0x01,0x0f,0x11,0x0f},
// 'b'
{0x10,0x10,0x1e,0x11,0x11,0x11,0x1e},
// 'c'
{0x00,0x00,0x0e,0x10,0x10,0x11,0x0e},
// 'd'
{0x01,0x01,0x0f,0x11,0x11,0x11,0x0f},
// 'e'
{0x00,0x00,0x0e,0x11,0x1f,0x10,0x0e},
// 'f'
{0x06,0x09,0x08,0x1e,0x08,0x08,0x08},
// 'g'
{0x00,0x0f,0x11,0x11,0x0f,0x01,0x0e},
// 'h'
{0x10,0x10,0x1e,0x11,0x11,0x11,0x11},
// 'i'
{0x04,0x00,0x0c,0x04,0x04,0x04,0x0e},
// 'j'
{0x02,0x00,0x06,0x02,0x02,0x12,0x0c},
// 'k'
{0x10,0x10,0x12,0x14,0x18,0x14,0x12},
// 'l'
{0x0c,0x04,0x04,0x04,0x04,0x04,0x0e},
// 'm'
{0x00,0x00,0x1a,0x15,0x15,0x11,0x11},
// 'n'
{0x00,0x00,0x16,0x19,0x11,0x11,0x11},
// 'o'
{0x00,0x00,0x0e,0x11,0x11,0x11,0x0e},
// 'p'
{0x00,0x1e,0x11,0x11,0x1e,0x10,0x10},
// 'q'
{0x00,0x0f,0x11,0x11,0x0f,0x01,0x01},
// 'r'
{0x00,0x00,0x16,0x19,0x10,0x10,0x10},
// 's'
{0x00,0x00,0x0e,0x10,0x0e,0x01,0x1e},
// 't'
{0x08,0x08,0x1e,0x08,0x08,0x09,0x06},
// 'u'
{0x00,0x00,0x11,0x11,0x11,0x13,0x0d},
// 'v'
{0x00,0x00,0x11,0x11,0x11,0x0a,0x04},
// 'w'
{0x00,0x00,0x11,0x15,0x15,0x15,0x0a},
// 'x'
{0x00,0x00,0x11,0x0a,0x04,0x0a,0x11},
// 'y'
{0x00,0x00,0x11,0x11,0x0f,0x01,0x0e},
// 'z'
{0x00,0x00,0x1f,0x02,0x04,0x08,0x1f},
// '{'
{0x06,0x08,0x08,0x10,0x08,0x08,0x06},
// '|'
{0x04,0x04,0x04,0x00,0x04,0x04,0x04},
// '}'
{0x0c,0x02,0x02,0x01,0x02,0x02,0x0c},
// '~'
{0x00,0x08,0x15,0x02,0x00,0x00,0x00},
};
// clang-format on
#endif

std::string backend_name(Backend backend) {
    switch (backend) {
    case Backend::Win32:
        return "Win32";
    case Backend::Cocoa:
        return "Cocoa";
    case Backend::X11:
        return "X11";
    case Backend::Headless:
        return "Headless";
    }
    return "Unknown";
}

Backend native_backend() {
    return detect_backend();
}

FullApplicationWindow::FullApplicationWindow(WindowConfig config)
    : impl_(new Impl{}) {
    impl_->config = std::move(config);
}

FullApplicationWindow::~FullApplicationWindow() {
    close();
    delete impl_;
}

bool FullApplicationWindow::create() {
    if (impl_->open) {
        return true;
    }

#if defined(_WIN32)
    impl_->instance = GetModuleHandle(nullptr);
    WNDCLASSA window_class{};
    window_class.lpfnWndProc = Impl::window_proc;
    window_class.hInstance = impl_->instance;
    window_class.lpszClassName = "CoolBoxFullApplicationWindow";
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassA(&window_class);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!impl_->config.resizable) {
        style &= ~static_cast<DWORD>(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    impl_->hwnd = CreateWindowExA(
        0,
        window_class.lpszClassName,
        impl_->config.title.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(impl_->config.width),
        static_cast<int>(impl_->config.height),
        nullptr,
        nullptr,
        impl_->instance,
        impl_);
    impl_->open = impl_->hwnd != nullptr;
    if (impl_->open) {
        impl_->rebuild_native_menu();
    }
    if (impl_->open && impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->hwnd), impl_->frame_index));
    }
    if (impl_->open && impl_->config.visible) {
        ShowWindow(impl_->hwnd, SW_SHOW);
        UpdateWindow(impl_->hwnd);
    }
    return impl_->open;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    {
        using CocoaInteger = long;
        using CocoaUnsignedInteger = unsigned long;
        using AppSharedFn               = id   (*)(id, SEL);
        using SetActivationPolicyFn     = void (*)(id, SEL, CocoaInteger);
        using AllocFn                   = id   (*)(id, SEL);
        using InitWindowFn              = id   (*)(id, SEL, CGRect, CocoaUnsignedInteger, CocoaUnsignedInteger, BOOL);
        using StringWithUtf8Fn          = id   (*)(id, SEL, const char*);
        using SetTitleFn                = void (*)(id, SEL, id);
        using VoidIdFn                  = void (*)(id, SEL, id);
        using VoidBoolFn                = void (*)(id, SEL, BOOL);
        using VoidVoidFn                = void (*)(id, SEL);

        fprintf(stderr, "[FAW] create(): sharedApplication\n");
        auto* ns_app = reinterpret_cast<AppSharedFn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSApplication")),
            sel_registerName("sharedApplication"));
        if (!ns_app) {
            fprintf(stderr, "[FAW] ERROR: sharedApplication returned nil\n");
            return false;
        }

        fprintf(stderr, "[FAW] create(): setActivationPolicy -> 0 (regular)\n");
        reinterpret_cast<SetActivationPolicyFn>(objc_msgSend)(
            ns_app, sel_registerName("setActivationPolicy:"), 0);

        // Drain any pending launch events before creating the window so
        // finishLaunching (called implicitly by makeKeyAndOrderFront:) does
        // not block waiting for run-loop sources.
        fprintf(stderr, "[FAW] create(): draining run loop before window init\n");
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);

        fprintf(stderr, "[FAW] create(): creating NSWindow\n");
        const CGRect rect = CGRectMake(0.0, 0.0,
            static_cast<double>(impl_->config.width),
            static_cast<double>(impl_->config.height));
        // Titled | Closable | Resizable
        const CocoaUnsignedInteger style = (1UL << 0U) | (1UL << 1U) | (1UL << 3U);
        auto* window = reinterpret_cast<AllocFn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSWindow")), sel_registerName("alloc"));
        window = reinterpret_cast<InitWindowFn>(objc_msgSend)(window,
            sel_registerName("initWithContentRect:styleMask:backing:defer:"),
            rect, style, 2UL, (BOOL)0);
        if (!window) {
            fprintf(stderr, "[FAW] ERROR: NSWindow init returned nil\n");
            return false;
        }
        fprintf(stderr, "[FAW] create(): NSWindow ok\n");
        reinterpret_cast<VoidBoolFn>(objc_msgSend)(
            window, sel_registerName("setReleasedWhenClosed:"), NO);

        const std::string safe_title = impl_->config.title.empty()
            ? std::string("Untitled") : impl_->config.title;
        auto* title_ns = reinterpret_cast<StringWithUtf8Fn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSString")),
            sel_registerName("stringWithUTF8String:"),
            safe_title.c_str());
        reinterpret_cast<SetTitleFn>(objc_msgSend)(window, sel_registerName("setTitle:"), title_ns);

        // Center before showing.
        reinterpret_cast<VoidVoidFn>(objc_msgSend)(window, sel_registerName("center"));

        // Order the window front; this also triggers finishLaunching internally
        // without blocking because we already drained the run loop above.
        fprintf(stderr, "[FAW] create(): makeKeyAndOrderFront:\n");
        reinterpret_cast<VoidIdFn>(objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), nil);

        fprintf(stderr, "[FAW] create(): activateIgnoringOtherApps:\n");
        reinterpret_cast<VoidBoolFn>(objc_msgSend)(ns_app, sel_registerName("activateIgnoringOtherApps:"), YES);

        // Give the window server a moment to composite the first frame.
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        fprintf(stderr, "[FAW] create(): done, window=%p\n", static_cast<void*>(window));

        impl_->application = ns_app;
        impl_->window = window;
        impl_->open = true;
        reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(
            window, sel_registerName("setAcceptsMouseMovedEvents:"), YES);
        impl_->rebuild_native_menu();
    }

    if (impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(
            reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
    }
    return impl_->open;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#if defined(GRAPHICS_HAVE_GTK3)
    if (!impl_->create_gtk_window()) return false;
    if (impl_->hooks.on_create)
        impl_->hooks.on_create(impl_->make_render_event(impl_->window, impl_->frame_index));
    return true;
#else
    impl_->display = XOpenDisplay(nullptr);
    if (!impl_->display) {
        impl_->backend = Backend::Headless;
        return false;
    }
    const int screen = DefaultScreen(impl_->display);
    impl_->window = XCreateSimpleWindow(impl_->display,
                                        RootWindow(impl_->display, screen),
                                        10,
                                        10,
                                        static_cast<unsigned int>(impl_->config.width),
                                        static_cast<unsigned int>(impl_->config.height),
                                        1,
                                        BlackPixel(impl_->display, screen),
                                        WhitePixel(impl_->display, screen));
    XStoreName(impl_->display, impl_->window, impl_->config.title.c_str());
    XSelectInput(impl_->display, impl_->window,
                 ExposureMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | StructureNotifyMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    impl_->delete_message = XInternAtom(impl_->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(impl_->display, impl_->window, &impl_->delete_message, 1);
    if (impl_->config.min_width > 0 || impl_->config.min_height > 0) {
        XSizeHints* hints = XAllocSizeHints();
        if (hints) {
            hints->flags = PMinSize;
            hints->min_width = static_cast<int>(impl_->config.min_width);
            hints->min_height = static_cast<int>(impl_->config.min_height);
            XSetWMNormalHints(impl_->display, impl_->window, hints);
            XFree(hints);
        }
    }
    if (impl_->config.visible) {
        XMapWindow(impl_->display, impl_->window);
        XFlush(impl_->display);
    }
    impl_->open = true;
    if (impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
    }
    return true;
#endif
#else
    impl_->backend = Backend::Headless;
    impl_->open = true;
    if (impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(0U, impl_->frame_index));
    }
    return true;
#endif
}

void FullApplicationWindow::show() {
    if (!impl_->open) {
        create();
    }
#if defined(_WIN32)
    if (impl_->hwnd) {
        ShowWindow(impl_->hwnd, SW_SHOW);
        UpdateWindow(impl_->hwnd);
    }
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (impl_->window) {
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(
            reinterpret_cast<id>(impl_->window), sel_registerName("makeKeyAndOrderFront:"), nil);
        // Re-activate the app so the window comes to the foreground.
        reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(
            reinterpret_cast<id>(impl_->application), sel_registerName("activateIgnoringOtherApps:"), YES);
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#if defined(GRAPHICS_HAVE_GTK3)
    if (impl_->gtk_window) gtk_widget_show(impl_->gtk_window);
#else
    if (impl_->display && impl_->window) {
        XMapWindow(impl_->display, impl_->window);
        XFlush(impl_->display);
    }
#endif
#endif
}

void FullApplicationWindow::close() {
    if (!impl_) {
        return;
    }
#if defined(_WIN32)
    if (impl_->hwnd) {
        impl_->clear_native_menu();
        impl_->release_ui_backbuffer();
        impl_->release_gl_resources();
        DestroyWindow(impl_->hwnd);
        impl_->hwnd = nullptr;
    }
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    detail::destroy_cocoa_menu(impl_->cocoa_menu);
    impl_->cocoa_menu = nullptr;
    if (impl_->window) {
        reinterpret_cast<void (*)(id, SEL)>(objc_msgSend)(reinterpret_cast<id>(impl_->window), sel_registerName("close"));
        reinterpret_cast<void (*)(id, SEL)>(objc_msgSend)(reinterpret_cast<id>(impl_->window), sel_registerName("release"));
        impl_->window = nullptr;
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#if defined(GRAPHICS_HAVE_GTK3)
    if (impl_->gtk_window) gtk_widget_destroy(impl_->gtk_window);
    if (impl_->gtk_accels) {
        g_object_unref(impl_->gtk_accels);
        impl_->gtk_accels = nullptr;
    }
    impl_->display = nullptr; // GDK owns the shared display connection.
    impl_->window = 0;
#else
    if (impl_->display && impl_->window) {
        XDestroyWindow(impl_->display, impl_->window);
        impl_->window = 0;
    }
    if (impl_->display) {
        XCloseDisplay(impl_->display);
        impl_->display = nullptr;
    }
#endif
#endif
    impl_->open = false;
}

bool FullApplicationWindow::pump_events() {
    if (!impl_->open) {
        return false;
    }

#if defined(_WIN32)
    MSG message{};
    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            impl_->open = false;
            return false;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    if (impl_->open && impl_->hooks.on_tick) {
        impl_->hooks.on_tick(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->hwnd), impl_->frame_index));
    }
    return impl_->open;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    {
        using AppSharedFn = id   (*)(id, SEL);
        using NextEventFn = id   (*)(id, SEL, unsigned long, id, id, BOOL);
        using SendEventFn = void (*)(id, SEL, id);
        using IsVisibleFn = BOOL (*)(id, SEL);

        auto* ns_app = reinterpret_cast<AppSharedFn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSApplication")),
            sel_registerName("sharedApplication"));

        auto* distant_past = reinterpret_cast<AppSharedFn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSDate")),
            sel_registerName("distantPast"));

        // Use kCFRunLoopDefaultMode string equivalent for NSDefaultRunLoopMode.
        auto* default_mode = reinterpret_cast<id (*)(id, SEL, const char*)>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSString")),
            sel_registerName("stringWithUTF8String:"),
            "NSDefaultRunLoopMode");

        // Also drain the CF run loop sources (timers, port messages, etc.)
        // that Cocoa defers but that NSApplication's event pump doesn't handle.
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, true);

        constexpr unsigned long k_any_event_mask = ~0UL;
        id event = nullptr;
        int drained = 0;
        while ((event = reinterpret_cast<NextEventFn>(objc_msgSend)(
                    ns_app,
                    sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
                    k_any_event_mask,
                    distant_past,
                    default_mode,
                    YES)) != nullptr) {
            using EventTypeFn = unsigned long (*)(id, SEL);
            using EventPointFn = CGPoint (*)(id, SEL);
            using EventDeltaFn = double (*)(id, SEL);
            const auto type = reinterpret_cast<EventTypeFn>(objc_msgSend)(event, sel_registerName("type"));
            if ((type == 1UL || type == 2UL || type == 3UL || type == 4UL ||
                 type == 5UL || type == 6UL || type == 7UL) &&
                reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(event, sel_registerName("window")) ==
                    reinterpret_cast<id>(impl_->window)) {
                const CGPoint location = reinterpret_cast<EventPointFn>(objc_msgSend)(
                    event, sel_registerName("locationInWindow"));
                int width = 0, height = 0;
                client_size(width, height);
                const auto flags = reinterpret_cast<EventTypeFn>(objc_msgSend)(
                    event, sel_registerName("modifierFlags"));
                if (type == 1UL) impl_->mac_control_click = (flags & (1UL << 18)) != 0;
                const PointerPressEvent pointer{
                    static_cast<int>(location.x), height - static_cast<int>(location.y),
                    (type == 3UL || type == 4UL || type == 7UL || impl_->mac_control_click)
                        ? PointerButton::Right : PointerButton::Left};
                if (type == 1UL || type == 3UL) {
                    if (impl_->hooks.on_pointer_press) impl_->hooks.on_pointer_press(pointer);
                } else if (type == 2UL || type == 4UL) {
                    if (type == 2UL) impl_->mac_control_click = false;
                    if (impl_->hooks.on_pointer_release) impl_->hooks.on_pointer_release(pointer);
                } else if (impl_->hooks.on_pointer_move) impl_->hooks.on_pointer_move(pointer);
            }
            if (type == 10UL && impl_->cocoa_menu) {
                const id menu = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(
                    ns_app, sel_registerName("mainMenu"));
                if (reinterpret_cast<BOOL (*)(id, SEL, id)>(objc_msgSend)(
                        menu, sel_registerName("performKeyEquivalent:"), event)) {
                    ++drained;
                    continue;
                }
            }
            if (type == 10UL && impl_->hooks.on_key &&
                reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(event, sel_registerName("window")) ==
                    reinterpret_cast<id>(impl_->window)) {
                const id characters = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(
                    event, sel_registerName("charactersIgnoringModifiers"));
                const auto length = reinterpret_cast<unsigned long (*)(id, SEL)>(objc_msgSend)(
                    characters, sel_registerName("length"));
                if (length > 0) {
                    const int character = reinterpret_cast<unsigned short (*)(id, SEL, unsigned long)>(objc_msgSend)(
                        characters, sel_registerName("characterAtIndex:"), 0);
                    int key = character;
                    if (key >= 'a' && key <= 'z') key -= 'a' - 'A';
                    switch (key) {
                        case 0xf702: key = static_cast<int>(Key::Left); break;
                        case 0xf703: key = static_cast<int>(Key::Right); break;
                        case 0xf729: key = static_cast<int>(Key::Home); break;
                        case 0xf72b: key = static_cast<int>(Key::End); break;
                        case 0xf728: key = static_cast<int>(Key::Delete); break;
                        case 127: key = static_cast<int>(Key::Backspace); break;
                        default: break;
                    }
                    const auto flags = reinterpret_cast<EventTypeFn>(objc_msgSend)(event, sel_registerName("modifierFlags"));
                    const bool repeat = reinterpret_cast<BOOL (*)(id, SEL)>(objc_msgSend)(
                        event, sel_registerName("isARepeat")) != NO;
                    impl_->hooks.on_key(KeyEvent{key, (flags & (1UL << 20)) != 0,
                        (flags & (1UL << 17)) != 0, repeat});
                    if (impl_->hooks.on_text_input && !(flags & (1UL << 20))) {
                        const id typed = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(
                            event, sel_registerName("characters"));
                        const char* utf8 = reinterpret_cast<const char* (*)(id, SEL)>(objc_msgSend)(
                            typed, sel_registerName("UTF8String"));
                        if (utf8 && *utf8 && static_cast<unsigned char>(*utf8) >= 32 &&
                            !(character >= 0xf700 && character <= 0xf8ff) && character != 127 &&
                            character != static_cast<int>(Key::Backspace))
                            impl_->hooks.on_text_input(utf8);
                    }
                }
                ++drained;
                continue;
            }
            if (impl_->hooks.on_scroll &&
                reinterpret_cast<EventTypeFn>(objc_msgSend)(
                    event, sel_registerName("type")) == 22UL) {
                const CGPoint location = reinterpret_cast<EventPointFn>(objc_msgSend)(
                    event, sel_registerName("locationInWindow"));
                const double delta = reinterpret_cast<EventDeltaFn>(objc_msgSend)(
                    event, sel_registerName("scrollingDeltaY"));
                impl_->hooks.on_scroll(ScrollEvent{
                    static_cast<int>(location.x),
                    static_cast<int>(impl_->config.height) - static_cast<int>(location.y),
                    delta, (reinterpret_cast<EventTypeFn>(objc_msgSend)(
                        event, sel_registerName("modifierFlags")) & (1UL << 17)) != 0});
            }
            reinterpret_cast<SendEventFn>(objc_msgSend)(
                ns_app, sel_registerName("sendEvent:"), event);
            ++drained;
        }
        if (drained > 0) {
            fprintf(stderr, "[FAW] pump_events: drained %d NS events\n", drained);
        }

        // Detect window close.
        if (impl_->window) {
            const BOOL visible = reinterpret_cast<IsVisibleFn>(objc_msgSend)(
                reinterpret_cast<id>(impl_->window), sel_registerName("isVisible"));
            if (!visible) {
                fprintf(stderr, "[FAW] pump_events: window not visible → closing\n");
                impl_->open = false;
                if (impl_->hooks.on_close) {
                    impl_->hooks.on_close(impl_->make_render_event(
                        reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
                }
                return false;
            }
        }
    }

    if (impl_->hooks.on_tick) {
        impl_->hooks.on_tick(impl_->make_render_event(
            reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
    }
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(
            reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
        impl_->mac_blit_to_window();
    }
    return impl_->open;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#if defined(GRAPHICS_HAVE_GTK3)
    while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
    if (impl_->open && impl_->hooks.on_tick)
        impl_->hooks.on_tick(impl_->make_render_event(impl_->window, impl_->frame_index));
    return impl_->open;
#else
    while (impl_->display && XPending(impl_->display) > 0) {
        XEvent event{};
        XNextEvent(impl_->display, &event);
        if ((event.type == ButtonPress || event.type == ButtonRelease) &&
            (event.xbutton.button == Button1 || event.xbutton.button == Button3)) {
            const PointerPressEvent pointer{event.xbutton.x, event.xbutton.y,
                event.xbutton.button == Button3 ? PointerButton::Right : PointerButton::Left};
            if (event.type == ButtonPress) {
                if (event.xbutton.button == Button1)
                    XGrabPointer(impl_->display, impl_->window, False,
                        ButtonReleaseMask | PointerMotionMask | ButtonPressMask,
                        GrabModeAsync, GrabModeAsync, None, None, event.xbutton.time);
                if (impl_->hooks.on_pointer_press) impl_->hooks.on_pointer_press(pointer);
            } else {
                if (event.xbutton.button == Button1) XUngrabPointer(impl_->display, event.xbutton.time);
                if (impl_->hooks.on_pointer_release) impl_->hooks.on_pointer_release(pointer);
            }
        }
        if (event.type == MotionNotify && impl_->hooks.on_pointer_move)
            impl_->hooks.on_pointer_move(PointerPressEvent{event.xmotion.x, event.xmotion.y,
                (event.xmotion.state & Button3Mask) ? PointerButton::Right : PointerButton::Left});
        if (event.type == FocusOut) impl_->held_keys.clear();
        if (event.type == KeyRelease) {
            bool repeat_release = false;
            if (XPending(impl_->display)) {
                XEvent next{};
                XPeekEvent(impl_->display, &next);
                repeat_release = next.type == KeyPress && next.xkey.keycode == event.xkey.keycode &&
                                 next.xkey.time == event.xkey.time;
            }
            if (!repeat_release) impl_->held_keys.erase(event.xkey.keycode);
        }
        if (event.type == KeyPress && impl_->hooks.on_key) {
            const bool repeat = !impl_->held_keys.insert(event.xkey.keycode).second;
            const KeySym symbol = XLookupKeysym(&event.xkey, 0);
            int key = static_cast<int>(symbol);
            if (key >= 'a' && key <= 'z') key -= 'a' - 'A';
            switch (symbol) {
                case XK_Left: key = static_cast<int>(Key::Left); break;
                case XK_Right: key = static_cast<int>(Key::Right); break;
                case XK_Home: key = static_cast<int>(Key::Home); break;
                case XK_End: key = static_cast<int>(Key::End); break;
                case XK_Delete: key = static_cast<int>(Key::Delete); break;
                case XK_BackSpace: key = static_cast<int>(Key::Backspace); break;
                case XK_Return: key = static_cast<int>(Key::Enter); break;
                case XK_Escape: key = static_cast<int>(Key::Escape); break;
                case XK_Tab:
                case XK_ISO_Left_Tab: key = 9; break;
                default: break;
            }
            impl_->hooks.on_key(KeyEvent{key, (event.xkey.state & ControlMask) != 0,
                (event.xkey.state & ShiftMask) != 0, repeat});
            if (impl_->hooks.on_text_input && !(event.xkey.state & (ControlMask | Mod1Mask))) {
                char text[64]{};
                KeySym typed{};
                const int length = XLookupString(&event.xkey, text, sizeof(text), &typed, nullptr);
                if (length > 0 && static_cast<unsigned char>(text[0]) >= 32 && text[0] != 127)
                    impl_->hooks.on_text_input(std::string(text, length));
            }
        }
        if (event.type == Expose && impl_->hooks.on_render) {
            impl_->hooks.on_render(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
        }
        if (event.type == ConfigureNotify) {
            impl_->config.width = static_cast<std::size_t>(event.xconfigure.width);
            impl_->config.height = static_cast<std::size_t>(event.xconfigure.height);
            if (impl_->hooks.on_resize) {
                impl_->hooks.on_resize(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
            }
        }
        if (event.type == ButtonPress && impl_->hooks.on_scroll &&
            (event.xbutton.button == Button4 || event.xbutton.button == Button5)) {
            impl_->hooks.on_scroll(ScrollEvent{
                event.xbutton.x,
                event.xbutton.y,
                event.xbutton.button == Button4 ? 1.0 : -1.0,
                (event.xbutton.state & ShiftMask) != 0});
        }
        if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == impl_->delete_message) {
            if (impl_->hooks.on_close) {
                impl_->hooks.on_close(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
            }
            impl_->open = false;
            return false;
        }
        if (event.type == DestroyNotify) {
            impl_->open = false;
            return false;
        }
    }
    if (impl_->open && impl_->hooks.on_tick) {
        impl_->hooks.on_tick(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
    }
    return impl_->open;
#endif
#else
    if (impl_->hooks.on_tick) {
        impl_->hooks.on_tick(impl_->make_render_event(0U, impl_->frame_index));
    }
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(0U, impl_->frame_index++));
    }
    return impl_->open;
#endif
}

bool FullApplicationWindow::is_open() const {
    return impl_->open;
}

Backend FullApplicationWindow::backend() const {
    return impl_->backend;
}

std::string FullApplicationWindow::backend_name() const {
    return full_application_window::backend_name(impl_->backend);
}

std::uintptr_t FullApplicationWindow::native_handle() const {
#if defined(_WIN32)
    return reinterpret_cast<std::uintptr_t>(impl_->hwnd);
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    return reinterpret_cast<std::uintptr_t>(impl_->window);
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    return static_cast<std::uintptr_t>(impl_->window);
#else
    return 0U;
#endif
}

const WindowConfig& FullApplicationWindow::config() const {
    return impl_->config;
}

void FullApplicationWindow::set_title(const std::string& title) {
    impl_->config.title = title;
#if defined(_WIN32)
    if (impl_->hwnd) {
        SetWindowTextA(impl_->hwnd, title.c_str());
    }
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (impl_->window) {
        const std::string safe_title = title.empty() ? std::string("Untitled") : title;
        auto* title_string = reinterpret_cast<id (*)(id, SEL, const char*)>(objc_msgSend)(reinterpret_cast<id>(objc_getClass("NSString")),
                                                                                            sel_registerName("stringWithUTF8String:"),
                                                                                            safe_title.c_str());
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(reinterpret_cast<id>(impl_->window), sel_registerName("setTitle:"), title_string);
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#if defined(GRAPHICS_HAVE_GTK3)
    if (impl_->gtk_window) gtk_window_set_title(GTK_WINDOW(impl_->gtk_window), title.c_str());
#else
    if (impl_->display && impl_->window) {
        XStoreName(impl_->display, impl_->window, title.c_str());
        XFlush(impl_->display);
    }
#endif
#endif
}

void FullApplicationWindow::set_menu_bar(const ::graphics::components::MenuBarModel& menu_bar) {
    impl_->menu_bar_model = menu_bar;
#if defined(_WIN32)
    if (impl_->hwnd) {
        impl_->rebuild_native_menu();
    }
#elif defined(GRAPHICS_HAVE_COCOA_RUNTIME) || defined(GRAPHICS_HAVE_GTK3)
    impl_->rebuild_native_menu();
#endif
}

bool FullApplicationWindow::has_native_menus() const {
#if defined(_WIN32)
    return impl_->backend == Backend::Win32;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    return impl_->backend == Backend::Cocoa;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_GTK3) && defined(GRAPHICS_HAVE_X11)
    return impl_->backend == Backend::X11;
#else
    return false;
#endif
}

const ::graphics::components::MenuBarModel& FullApplicationWindow::menu_bar() const {
    return impl_->menu_bar_model;
}

void FullApplicationWindow::set_menu_command_handler(MenuCommandHandler handler) {
    impl_->menu_command_handler = std::move(handler);
}

const MenuCommandHandler& FullApplicationWindow::menu_command_handler() const {
    return impl_->menu_command_handler;
}

bool FullApplicationWindow::set_menu_item_checked(std::size_t menu_index, std::size_t item_index, bool checked) {
    if (menu_index >= impl_->menu_bar_model.menus.size()) {
        return false;
    }
    auto& menu = impl_->menu_bar_model.menus[menu_index];
    if (item_index >= menu.items.size()) {
        return false;
    }
    auto& item = menu.items[item_index];
    if (item.separator) {
        return false;
    }
    if (item.checked == checked) {
        return true;
    }
    item.checked = checked;
#if defined(_WIN32)
    if (impl_->hwnd) {
        impl_->rebuild_native_menu();
    }
#elif defined(GRAPHICS_HAVE_COCOA_RUNTIME) || defined(GRAPHICS_HAVE_GTK3)
    impl_->rebuild_native_menu();
#endif
    return true;
}

bool FullApplicationWindow::menu_item_checked(std::size_t menu_index, std::size_t item_index) const {
    if (menu_index >= impl_->menu_bar_model.menus.size()) {
        return false;
    }
    const auto& menu = impl_->menu_bar_model.menus[menu_index];
    if (item_index >= menu.items.size()) {
        return false;
    }
    const auto& item = menu.items[item_index];
    if (item.separator) {
        return false;
    }
    return item.checked;
}

void FullApplicationWindow::set_platform_style(::graphics::windows::PlatformStyle platform_style) {
    impl_->platform = platform_style;
}

::graphics::windows::PlatformStyle FullApplicationWindow::platform_style() const {
    return impl_->platform;
}

void FullApplicationWindow::set_render_hooks(RenderHooks hooks) {
    impl_->hooks = std::move(hooks);
}

const RenderHooks& FullApplicationWindow::render_hooks() const {
    return impl_->hooks;
}

void FullApplicationWindow::request_redraw() {
#if defined(_WIN32)
    if (impl_->hwnd) {
        InvalidateRect(impl_->hwnd, nullptr, FALSE);
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    // Render synchronously (matching the Cocoa/headless backends below)
    // rather than XClearArea()-then-wait-for-the-next-Expose-event: that
    // older approach blanked the window to the background pixel immediately
    // and only repainted whenever the next Expose happened to arrive,
    // producing a clear -> (gap) -> redraw "strobe" where the window was
    // blank the vast majority of the time.
    if (impl_->display && impl_->window && impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(
            static_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
    }
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(
            reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
        impl_->mac_blit_to_window();
    }
#else
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(0U, impl_->frame_index++));
    }
#endif
}

bool FullApplicationWindow::query_pointer_state(PointerState& state) const {
#if defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (!impl_->window) {
        state = PointerState{};
        return false;
    }
    {
        // Mouse location in NSWindow base coordinates (origin = bottom-left).
        using CGPointFn = CGPoint (*)(id, SEL);
        using CGRectFn  = CGRect  (*)(id, SEL);
        using ULongFn   = unsigned long (*)(id, SEL);
        using IdFn      = id (*)(id, SEL);

        const CGPoint mouse_loc = reinterpret_cast<CGPointFn>(objc_msgSend)(
            reinterpret_cast<id>(impl_->window),
            sel_registerName("mouseLocationOutsideOfEventStream"));

        auto* content_view = reinterpret_cast<IdFn>(objc_msgSend)(
            reinterpret_cast<id>(impl_->window), sel_registerName("contentView"));

        const CGRect bounds = reinterpret_cast<CGRectFn>(objc_msgSend)(
            content_view, sel_registerName("bounds"));

        // Flip y: NSWindow has origin at bottom-left, UI expects top-left origin.
        state.client_width  = static_cast<int>(bounds.size.width);
        state.client_height = static_cast<int>(bounds.size.height);
        state.x = static_cast<int>(mouse_loc.x);
        state.y = state.client_height - static_cast<int>(mouse_loc.y);
        state.inside = mouse_loc.x >= 0.0 && mouse_loc.x <= bounds.size.width
                    && mouse_loc.y >= 0.0 && mouse_loc.y <= bounds.size.height;

        const unsigned long pressed = reinterpret_cast<ULongFn>(objc_msgSend)(
            reinterpret_cast<id>(objc_getClass("NSEvent")),
            sel_registerName("pressedMouseButtons"));
        state.left_button_down  = (pressed & 1U) != 0 && !impl_->mac_control_click;
        state.right_button_down = (pressed & 2U) != 0 ||
                                  ((pressed & 1U) != 0 && impl_->mac_control_click);
        using BoolFn = bool (*)(id, SEL);
        state.focused = reinterpret_cast<BoolFn>(objc_msgSend)(
            reinterpret_cast<id>(impl_->window), sel_registerName("isKeyWindow"));
    }
    return true;
#elif defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return false;
    }

    POINT cursor{};
    if (!GetCursorPos(&cursor)) {
        return false;
    }
    ScreenToClient(impl_->hwnd, &cursor);

    RECT client_rect{};
    GetClientRect(impl_->hwnd, &client_rect);
    const int width = std::max(0, static_cast<int>(client_rect.right - client_rect.left));
    const int height = std::max(0, static_cast<int>(client_rect.bottom - client_rect.top));

    state.x = cursor.x;
    state.y = cursor.y;
    state.client_width = width;
    state.client_height = height;
    state.inside = cursor.x >= 0 && cursor.y >= 0 && cursor.x < width && cursor.y < height;
    state.left_button_down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    state.right_button_down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    state.focused = GetForegroundWindow() == impl_->hwnd;
    return true;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    if (!impl_->display || !impl_->window) {
        state = PointerState{};
        return false;
    }
    ::Window root_return = 0;
    ::Window child_return = 0;
    int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
    unsigned int mask = 0;
    if (!XQueryPointer(impl_->display, impl_->window, &root_return, &child_return,
                       &root_x, &root_y, &win_x, &win_y, &mask)) {
        state = PointerState{};
        return false;
    }
    int width = 0, height = 0;
    client_size(width, height);
    state.x = win_x;
    state.y = win_y;
    state.client_width = width;
    state.client_height = height;
    state.inside = win_x >= 0 && win_y >= 0 && win_x < width && win_y < height;
    state.left_button_down = (mask & Button1Mask) != 0;
    state.right_button_down = (mask & Button3Mask) != 0;
    ::Window focused = 0;
    int revert = 0;
    XGetInputFocus(impl_->display, &focused, &revert);
    state.focused = focused == impl_->window;
#if defined(GRAPHICS_HAVE_GTK3)
    // GTK normally gives X input focus to its toplevel, not the drawable child XID.
    state.focused = impl_->gtk_window && gtk_window_is_active(GTK_WINDOW(impl_->gtk_window));
#endif
    return true;
#else
    state = PointerState{};
    return false;
#endif
}

void FullApplicationWindow::show_info_dialog(const std::string& title, const std::string& message) const {
#if defined(_WIN32)
    if (impl_->hwnd && IsWindow(impl_->hwnd)) {
        MessageBoxA(impl_->hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    }
#else
    (void)title;
    (void)message;
#endif
}

bool FullApplicationWindow::client_size(int& width, int& height) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        width = 0;
        height = 0;
        return false;
    }
    RECT rc{};
    GetClientRect(impl_->hwnd, &rc);
    width = std::max(0, static_cast<int>(rc.right - rc.left));
    height = std::max(0, static_cast<int>(rc.bottom - rc.top));
    return true;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (!impl_->window) {
        width = 0;
        height = 0;
        return false;
    }
    {
        using IdFn     = id     (*)(id, SEL);
        using CGRectFn = CGRect (*)(id, SEL);
        auto* content_view = reinterpret_cast<IdFn>(objc_msgSend)(
            reinterpret_cast<id>(impl_->window), sel_registerName("contentView"));
        const CGRect bounds = reinterpret_cast<CGRectFn>(objc_msgSend)(
            content_view, sel_registerName("bounds"));
        width  = static_cast<int>(bounds.size.width);
        height = static_cast<int>(bounds.size.height);
    }
    return true;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    if (!impl_->display || !impl_->window) {
        width = 0;
        height = 0;
        return false;
    }
    XWindowAttributes attrs{};
    XGetWindowAttributes(impl_->display, impl_->window, &attrs);
    width = attrs.width;
    height = attrs.height;
    return true;
#else
    width = 0;
    height = 0;
    return false;
#endif
}

void FullApplicationWindow::clear_background(unsigned char r, unsigned char g, unsigned char b) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return;
    }
    RECT rc{};
    GetClientRect(impl_->hwnd, &rc);
    const int width = std::max(0, static_cast<int>(rc.right - rc.left));
    const int height = std::max(0, static_cast<int>(rc.bottom - rc.top));
    if (!impl_->ensure_ui_backbuffer(width, height)) {
        return;
    }

    HDC dc = impl_->ui_backbuffer_dc;
    HBRUSH bg = CreateSolidBrush(RGB(r, g, b));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);
    impl_->ui_backbuffer_used_this_frame = true;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    {
        int w = 0, h = 0;
        client_size(w, h);
        impl_->mac_ensure_buf(w, h);
        impl_->mac_fill_rect(0, 0, w, h, r, g, b);
    }
#else
    (void)r;
    (void)g;
    (void)b;
#endif
}

void FullApplicationWindow::fill_rect(int left, int top, int right, int bottom,
                                      unsigned char r, unsigned char g, unsigned char b) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return;
    }
    RECT client{};
    GetClientRect(impl_->hwnd, &client);
    const int width = std::max(0, static_cast<int>(client.right - client.left));
    const int height = std::max(0, static_cast<int>(client.bottom - client.top));
    if (!impl_->ensure_ui_backbuffer(width, height)) {
        return;
    }

    HDC dc = impl_->ui_backbuffer_dc;
    RECT rect{left, top, right, bottom};
    HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
    impl_->ui_backbuffer_used_this_frame = true;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    impl_->mac_fill_rect(left, top, right, bottom, r, g, b);
#else
    (void)left;
    (void)top;
    (void)right;
    (void)bottom;
    (void)r;
    (void)g;
    (void)b;
#endif
}

void FullApplicationWindow::draw_arc(int cx, int cy, int radius,
                                     double start_angle_deg, double end_angle_deg,
                                     unsigned char r, unsigned char g, unsigned char b,
                                     int thickness) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd) || radius <= 0 || thickness <= 0) {
        return;
    }
    RECT client{};
    GetClientRect(impl_->hwnd, &client);
    const int width = std::max(0, static_cast<int>(client.right - client.left));
    const int height = std::max(0, static_cast<int>(client.bottom - client.top));
    if (!impl_->ensure_ui_backbuffer(width, height)) {
        return;
    }

    auto normalize_deg = [](double deg) {
        double out = std::fmod(deg, 360.0);
        if (out < 0.0) out += 360.0;
        return out;
    };

    double start = normalize_deg(start_angle_deg);
    double end = normalize_deg(end_angle_deg);
    if (end < start || (std::abs(end - start) < 1e-9 && std::abs(end_angle_deg - start_angle_deg) > 1e-9)) {
        end += 360.0;
    }

    const double span = end - start;
    const int segments = std::max(12, static_cast<int>(std::ceil((span / 360.0) * radius * 8.0)));
    const double step = span / static_cast<double>(segments);

    HDC dc = impl_->ui_backbuffer_dc;
    HPEN pen = CreatePen(PS_SOLID, thickness, RGB(r, g, b));
    HGDIOBJ old_pen = SelectObject(dc, pen);

    bool first = true;
    int prev_x = 0;
    int prev_y = 0;
    for (int i = 0; i <= segments; ++i) {
        const double deg = start + step * static_cast<double>(i);
        const double rad = deg * 3.14159265358979323846 / 180.0;
        const int px = cx + static_cast<int>(std::lround(std::cos(rad) * radius));
        const int py = cy + static_cast<int>(std::lround(std::sin(rad) * radius));
        if (first) {
            MoveToEx(dc, px, py, nullptr);
            first = false;
        } else {
            MoveToEx(dc, prev_x, prev_y, nullptr);
            LineTo(dc, px, py);
        }
        prev_x = px;
        prev_y = py;
    }

    SelectObject(dc, old_pen);
    DeleteObject(pen);
    impl_->ui_backbuffer_used_this_frame = true;
#else
    (void)cx;
    (void)cy;
    (void)radius;
    (void)start_angle_deg;
    (void)end_angle_deg;
    (void)r;
    (void)g;
    (void)b;
    (void)thickness;
#endif
}

void FullApplicationWindow::draw_rounded_rect(int left, int top, int right, int bottom,
                                              int radius,
                                              unsigned char r, unsigned char g, unsigned char b,
                                              bool filled) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd) || right <= left || bottom <= top) {
        return;
    }
    RECT client{};
    GetClientRect(impl_->hwnd, &client);
    const int width = std::max(0, static_cast<int>(client.right - client.left));
    const int height = std::max(0, static_cast<int>(client.bottom - client.top));
    if (!impl_->ensure_ui_backbuffer(width, height)) {
        return;
    }

    const int rw = std::max(1, std::min(radius, std::max(1, (right - left) / 2)) * 2);
    const int rh = std::max(1, std::min(radius, std::max(1, (bottom - top) / 2)) * 2);

    HDC dc = impl_->ui_backbuffer_dc;
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
    HGDIOBJ old_pen = SelectObject(dc, pen);

    HBRUSH brush = nullptr;
    HGDIOBJ old_brush = nullptr;
    if (filled) {
        brush = CreateSolidBrush(RGB(r, g, b));
        old_brush = SelectObject(dc, brush);
    } else {
        old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    }

    RoundRect(dc, left, top, right, bottom, rw, rh);

    SelectObject(dc, old_pen);
    DeleteObject(pen);
    SelectObject(dc, old_brush);
    if (brush) {
        DeleteObject(brush);
    }
    impl_->ui_backbuffer_used_this_frame = true;
#else
    (void)left;
    (void)top;
    (void)right;
    (void)bottom;
    (void)radius;
    (void)r;
    (void)g;
    (void)b;
    (void)filled;
#endif
}

void FullApplicationWindow::draw_text_line(int x, int y, const std::string& text,
                                           unsigned char r, unsigned char g, unsigned char b) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return;
    }
    RECT client{};
    GetClientRect(impl_->hwnd, &client);
    const int width = std::max(0, static_cast<int>(client.right - client.left));
    const int height = std::max(0, static_cast<int>(client.bottom - client.top));
    if (!impl_->ensure_ui_backbuffer(width, height)) {
        return;
    }

    HDC dc = impl_->ui_backbuffer_dc;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(r, g, b));
    TextOutA(dc, x, y, text.c_str(), static_cast<int>(text.size()));
    impl_->ui_backbuffer_used_this_frame = true;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    impl_->mac_draw_text(x, y, text, r, g, b);
#else
    (void)x;
    (void)y;
    (void)text;
    (void)r;
    (void)g;
    (void)b;
#endif
}

bool FullApplicationWindow::present_canvas(const ::graphics::Canvas& canvas) const {
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return false;
    }

    int width = 0;
    int height = 0;
    if (!client_size(width, height) || width <= 0 || height <= 0) {
        return false;
    }

    if (impl_->ensure_gl_renderer()) {
        if (wglMakeCurrent(impl_->gl_hdc, impl_->gl_context)) {
            glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, impl_->gl_texture);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                static_cast<GLsizei>(canvas.width()),
                static_cast<GLsizei>(canvas.height()),
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                canvas.data());

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
            glEnd();

            SwapBuffers(impl_->gl_hdc);
            return true;
        }
    }

    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = canvas.width();
    bitmap_info.bmiHeader.biHeight = -canvas.height();
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    HDC dc = GetDC(impl_->hwnd);
    if (!dc) {
        return false;
    }

    StretchDIBits(dc,
                  0,
                  0,
                  width,
                  height,
                  0,
                  0,
                  canvas.width(),
                  canvas.height(),
                  canvas.data(),
                  &bitmap_info,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    ReleaseDC(impl_->hwnd, dc);
    return true;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    if (!impl_->display || !impl_->window) return false;

    const int w = canvas.width();
    const int h = canvas.height();
    if (w <= 0 || h <= 0) return false;

    const int screen = DefaultScreen(impl_->display);
    Visual* visual = DefaultVisual(impl_->display, screen);
    const int depth = DefaultDepth(impl_->display, screen);
    if (depth != 24 && depth != 32) return false; // uncommon visual depth; bail out safely

    auto shift_for_mask = [](unsigned long mask) {
        int shift = 0;
        while (mask != 0 && (mask & 1UL) == 0) { mask >>= 1; ++shift; }
        return shift;
    };
    const int rshift = shift_for_mask(visual->red_mask);
    const int gshift = shift_for_mask(visual->green_mask);
    const int bshift = shift_for_mask(visual->blue_mask);

    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    const unsigned char* src = canvas.data(); // RGBA8, row-major, top-left origin
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        const unsigned char r = src[i * 4 + 0];
        const unsigned char g = src[i * 4 + 1];
        const unsigned char b = src[i * 4 + 2];
        pixels[i] = (static_cast<std::uint32_t>(r) << rshift) |
                   (static_cast<std::uint32_t>(g) << gshift) |
                   (static_cast<std::uint32_t>(b) << bshift);
    }

    XImage* image = XCreateImage(impl_->display, visual, static_cast<unsigned int>(depth), ZPixmap, 0,
                                 reinterpret_cast<char*>(pixels.data()),
                                 static_cast<unsigned int>(w), static_cast<unsigned int>(h), 32, 0);
    if (!image) return false;

    GC gc = XCreateGC(impl_->display, impl_->window, 0, nullptr);
    XPutImage(impl_->display, impl_->window, gc, image, 0, 0, 0, 0,
             static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    XFreeGC(impl_->display, gc);

    image->data = nullptr; // `pixels` owns the buffer; don't let XDestroyImage free it
    XDestroyImage(image);
    XFlush(impl_->display);
    return true;
#else
    (void)canvas;
    return false;
#endif
}

void show_canvas_window(const ::graphics::Canvas& canvas,
                        const std::string& title,
                        const std::string& save_path) {
    if (save_path.empty()) {
        throw std::invalid_argument("plot save path must not be empty");
    }
    if (save_path.find('\0') != std::string::npos) {
        throw std::invalid_argument("plot save path must not contain a NUL byte");
    }

    FullApplicationWindow window(WindowConfig{
        title.empty() ? "CoolBox Plot" : title,
        static_cast<std::size_t>(canvas.width()),
        static_cast<std::size_t>(canvas.height()),
        true,
        true});

    double zoom = 1.0;
    double pan_x = 0.0;
    double pan_y = 0.0;
    bool dragging = false;
    bool pointer_was_down = false;
    int previous_pointer_x = 0;
    int previous_pointer_y = 0;
    int view_width = canvas.width();
    int view_height = canvas.height();
    constexpr int save_button_width = 86;
    constexpr int save_button_height = 26;
    constexpr int save_button_margin = 8;

    const auto render = [&] {
        if (view_width <= 0 || view_height <= 0) {
            return;
        }

        ::graphics::Canvas view(view_width, view_height, ::graphics::Colors::White);
        const double scale = std::min(
            static_cast<double>(view_width) / canvas.width(),
            static_cast<double>(view_height) / canvas.height()) * zoom;
        const double center_x = view_width / 2.0 + pan_x;
        const double center_y = view_height / 2.0 + pan_y;
        for (int y = 0; y < view_height; ++y) {
            const int source_y = static_cast<int>(
                std::floor((y - center_y) / scale + canvas.height() / 2.0));
            if (source_y < 0 || source_y >= canvas.height()) {
                continue;
            }
            for (int x = 0; x < view_width; ++x) {
                const int source_x = static_cast<int>(
                    std::floor((x - center_x) / scale + canvas.width() / 2.0));
                if (source_x >= 0 && source_x < canvas.width()) {
                    view.set_pixel(x, y, canvas.get_pixel(source_x, source_y));
                }
            }
        }

        const int button_x = std::max(
            0, view_width - save_button_width - save_button_margin);
        view.draw_rounded_rect(button_x, save_button_margin,
                               save_button_width, save_button_height, 4,
                               ::graphics::Colors::DarkGray, true);
        view.draw_text(button_x + 10, save_button_margin + 9,
                       "Save PNG", ::graphics::Colors::White);
        view.draw_text(save_button_margin, view_height - 14,
                       "Wheel: zoom  |  Drag: pan",
                       ::graphics::Colors::DarkGray);
        window.present_canvas(view);
    };

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent& event) {
        view_width = static_cast<int>(event.width);
        view_height = static_cast<int>(event.height);
        render();
    };
    hooks.on_resize = [&](const RenderEvent& event) {
        view_width = static_cast<int>(event.width);
        view_height = static_cast<int>(event.height);
        window.request_redraw();
    };
    hooks.on_scroll = [&](const ScrollEvent& event) {
        if (!std::isfinite(event.delta) || event.delta == 0.0 ||
            view_width <= 0 || view_height <= 0) {
            return;
        }
        const double old_scale = std::min(
            static_cast<double>(view_width) / canvas.width(),
            static_cast<double>(view_height) / canvas.height()) * zoom;
        const double source_x =
            (event.x - view_width / 2.0 - pan_x) / old_scale + canvas.width() / 2.0;
        const double source_y =
            (event.y - view_height / 2.0 - pan_y) / old_scale + canvas.height() / 2.0;
        zoom = std::clamp(zoom * std::pow(1.15, event.delta), 0.25, 16.0);
        const double new_scale = std::min(
            static_cast<double>(view_width) / canvas.width(),
            static_cast<double>(view_height) / canvas.height()) * zoom;
        pan_x = event.x - view_width / 2.0 -
                (source_x - canvas.width() / 2.0) * new_scale;
        pan_y = event.y - view_height / 2.0 -
                (source_y - canvas.height() / 2.0) * new_scale;
        window.request_redraw();
    };
    window.set_render_hooks(std::move(hooks));

    if (!window.create() || window.backend() == Backend::Headless) {
        window.close();
        throw std::runtime_error("no native window backend is available");
    }
    window.show();

    while (window.pump_events()) {
        PointerState pointer;
        if (window.query_pointer_state(pointer)) {
            const int button_x = std::max(
                0, pointer.client_width - save_button_width - save_button_margin);
            const bool on_save_button =
                pointer.x >= button_x &&
                pointer.x < button_x + save_button_width &&
                pointer.y >= save_button_margin &&
                pointer.y < save_button_margin + save_button_height;
            if (pointer.left_button_down && !pointer_was_down) {
                if (on_save_button) {
                    if (!canvas.save_png(save_path)) {
                        window.show_info_dialog("Save failed",
                            "Could not save plot to " + save_path);
                    } else {
                        window.show_info_dialog("Plot saved",
                            "Saved plot to " + save_path);
                    }
                    dragging = false;
                } else {
                    dragging = true;
                    previous_pointer_x = pointer.x;
                    previous_pointer_y = pointer.y;
                }
            } else if (pointer.left_button_down && dragging) {
                const int dx = pointer.x - previous_pointer_x;
                const int dy = pointer.y - previous_pointer_y;
                if (dx != 0 || dy != 0) {
                    pan_x += dx;
                    pan_y += dy;
                    previous_pointer_x = pointer.x;
                    previous_pointer_y = pointer.y;
                    window.request_redraw();
                }
            } else if (!pointer.left_button_down) {
                dragging = false;
            }
            pointer_was_down = pointer.left_button_down;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

} // namespace full_application_window
} // namespace graphics