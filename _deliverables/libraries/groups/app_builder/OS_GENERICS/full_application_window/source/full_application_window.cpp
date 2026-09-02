#include "full_application_window.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <stdexcept>
#include <utility>

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
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
    ::Window window = 0;
    Atom delete_message = 0;
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

        if (impl && message == WM_SIZE) {
            impl->config.width = static_cast<std::size_t>(LOWORD(lparam));
            impl->config.height = static_cast<std::size_t>(HIWORD(lparam));
            impl->release_ui_backbuffer();
            if (impl->hooks.on_resize) {
                impl->hooks.on_resize(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index));
            }
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

        if (impl && message == WM_COMMAND) {
            const UINT command_id = LOWORD(wparam);
            const auto found = impl->command_to_menu_item.find(command_id);
            if (found != impl->command_to_menu_item.end()) {
                const std::size_t menu_index = found->second.first;
                const std::size_t item_index = found->second.second;
                if (menu_index < impl->menu_bar_model.menus.size()) {
                    auto& menu = impl->menu_bar_model.menus[menu_index];
                    if (item_index < menu.items.size()) {
                        const auto& item = menu.items[item_index];
                        if (impl->menu_command_handler) {
                            impl->menu_command_handler(menu_index, item_index, menu, item);
                        }
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
                AppendMenuA(popup, flags, command_id, item.label.c_str());
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
    }

    if (impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(
            reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
    }
    return impl_->open;
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
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
    XSelectInput(impl_->display, impl_->window, ExposureMask | KeyPressMask | StructureNotifyMask);
    impl_->delete_message = XInternAtom(impl_->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(impl_->display, impl_->window, &impl_->delete_message, 1);
    if (impl_->config.visible) {
        XMapWindow(impl_->display, impl_->window);
        XFlush(impl_->display);
    }
    impl_->open = true;
    if (impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(static_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
    }
    return true;
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
    if (impl_->display && impl_->window) {
        XMapWindow(impl_->display, impl_->window);
        XFlush(impl_->display);
    }
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
    if (impl_->window) {
        reinterpret_cast<void (*)(id, SEL)>(objc_msgSend)(reinterpret_cast<id>(impl_->window), sel_registerName("close"));
        impl_->window = nullptr;
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    if (impl_->display && impl_->window) {
        XDestroyWindow(impl_->display, impl_->window);
        impl_->window = 0;
    }
    if (impl_->display) {
        XCloseDisplay(impl_->display);
        impl_->display = nullptr;
    }
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
    while (impl_->display && XPending(impl_->display) > 0) {
        XEvent event{};
        XNextEvent(impl_->display, &event);
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
    if (impl_->display && impl_->window) {
        XStoreName(impl_->display, impl_->window, title.c_str());
        XFlush(impl_->display);
    }
#endif
}

void FullApplicationWindow::set_menu_bar(const ::graphics::components::MenuBarModel& menu_bar) {
    impl_->menu_bar_model = menu_bar;
#if defined(_WIN32)
    if (impl_->hwnd) {
        impl_->rebuild_native_menu();
    }
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
    if (impl_->display && impl_->window) {
        XClearArea(impl_->display, impl_->window, 0, 0, 0, 0, True);
        XFlush(impl_->display);
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
        state.left_button_down  = (pressed & 1U) != 0;
        state.right_button_down = (pressed & 2U) != 0;
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
#else
    (void)canvas;
    return false;
#endif
}

} // namespace full_application_window
} // namespace graphics