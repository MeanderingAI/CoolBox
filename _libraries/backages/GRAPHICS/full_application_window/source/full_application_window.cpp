#include "full_application_window.hpp"

#include <stdexcept>
#include <utility>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
#include <CoreGraphics/CoreGraphics.h>
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
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    void* application = nullptr;
    void* window = nullptr;
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
            if (impl->hooks.on_resize) {
                impl->hooks.on_resize(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index));
            }
            return 0;
        }

        if (impl && message == WM_PAINT) {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            if (impl->hooks.on_render) {
                impl->hooks.on_render(impl->make_render_event(reinterpret_cast<std::uintptr_t>(hwnd), impl->frame_index++));
            }
            EndPaint(hwnd, &paint);
            return 0;
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
#endif
};

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
    if (impl_->open && impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->hwnd), impl_->frame_index));
    }
    if (impl_->open && impl_->config.visible) {
        ShowWindow(impl_->hwnd, SW_SHOW);
        UpdateWindow(impl_->hwnd);
    }
    return impl_->open;
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    using MsgSend = id (*)(id, SEL, ...);
    using CocoaInteger = long;
    using CocoaUnsignedInteger = unsigned long;
    auto* ns_application = reinterpret_cast<MsgSend>(objc_msgSend)(reinterpret_cast<id>(objc_getClass("NSApplication")), sel_registerName("sharedApplication"));
    reinterpret_cast<void (*)(id, SEL, CocoaInteger)>(objc_msgSend)(ns_application, sel_registerName("setActivationPolicy:"), 0);

    const CGRect rect = CGRectMake(0.0, 0.0, static_cast<double>(impl_->config.width), static_cast<double>(impl_->config.height));
    const CocoaUnsignedInteger style = (1UL << 0U) | (1UL << 1U) | (1UL << 3U);
    auto* window = reinterpret_cast<MsgSend>(objc_msgSend)(reinterpret_cast<id>(objc_getClass("NSWindow")), sel_registerName("alloc"));
    window = reinterpret_cast<MsgSend>(objc_msgSend)(window,
                                                     sel_registerName("initWithContentRect:styleMask:backing:defer:"),
                                                     rect,
                                                     style,
                                                     2UL,
                                                     false);

    auto* title_string = reinterpret_cast<MsgSend>(objc_msgSend)(reinterpret_cast<id>(objc_getClass("NSString")),
                                                                 sel_registerName("stringWithUTF8String:"),
                                                                 impl_->config.title.c_str());
    reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(window, sel_registerName("setTitle:"), title_string);
    if (impl_->config.visible) {
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), nil);
        reinterpret_cast<void (*)(id, SEL, BOOL)>(objc_msgSend)(ns_application, sel_registerName("activateIgnoringOtherApps:"), YES);
    }

    impl_->application = ns_application;
    impl_->window = window;
    impl_->open = window != nullptr;
    if (impl_->open && impl_->hooks.on_create) {
        impl_->hooks.on_create(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index));
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
        reinterpret_cast<void (*)(id, SEL, id)>(objc_msgSend)(reinterpret_cast<id>(impl_->window), sel_registerName("makeKeyAndOrderFront:"), nil);
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
    if (impl_->hooks.on_tick) {
        impl_->hooks.on_tick(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
    }
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
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
        auto* title_string = reinterpret_cast<id (*)(id, SEL, const char*)>(objc_msgSend)(reinterpret_cast<id>(objc_getClass("NSString")),
                                                                                            sel_registerName("stringWithUTF8String:"),
                                                                                            title.c_str());
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
}

const ::graphics::components::MenuBarModel& FullApplicationWindow::menu_bar() const {
    return impl_->menu_bar_model;
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
        InvalidateRect(impl_->hwnd, nullptr, TRUE);
    }
#elif defined(__linux__) && defined(GRAPHICS_HAVE_X11)
    if (impl_->display && impl_->window) {
        XClearArea(impl_->display, impl_->window, 0, 0, 0, 0, True);
        XFlush(impl_->display);
    }
#elif defined(__APPLE__) && defined(GRAPHICS_HAVE_COCOA_RUNTIME)
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(reinterpret_cast<std::uintptr_t>(impl_->window), impl_->frame_index++));
    }
#else
    if (impl_->hooks.on_render) {
        impl_->hooks.on_render(impl_->make_render_event(0U, impl_->frame_index++));
    }
#endif
}

} // namespace full_application_window
} // namespace graphics