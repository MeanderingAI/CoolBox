#include "tyst_framework.hpp"

#include <type_traits>
#include <chrono>
#include <thread>

#include "full_application_window.hpp"
#include "workspace_dock_host.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef GRAPHICS_TEST_GTK3
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

namespace {

void assert_graphics_object_contract(const graphics::GraphicsObject& object,
                                     const std::string& expected_kind,
                                     const std::string& expected_name) {
    EXPECT_EQ(object.graphics_object_kind(), expected_kind);
    EXPECT_EQ(object.graphics_object_name(), expected_name);
}

static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::WindowConfig>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::RenderEvent>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::WorkspacePanelLayout>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::WorkspaceNavItem>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::WorkspaceDockModels>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::FullApplicationWindow>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::full_application_window::WorkspaceDockHost>);

} // namespace

TEST(FullApplicationWindowTest, ReportsNativeBackendForCurrentPlatform) {
    const auto backend = graphics::full_application_window::native_backend();
#if defined(_WIN32)
    EXPECT_EQ(backend, graphics::full_application_window::Backend::Win32);
#elif defined(__APPLE__)
    EXPECT_EQ(backend, graphics::full_application_window::Backend::Cocoa);
#elif defined(__linux__)
    EXPECT_TRUE(backend == graphics::full_application_window::Backend::X11 ||
                backend == graphics::full_application_window::Backend::Headless);
#else
    EXPECT_EQ(backend, graphics::full_application_window::Backend::Headless);
#endif
}

TEST(FullApplicationWindowTest, PreservesConfigAndMenuBarState) {
    graphics::full_application_window::WindowConfig config;
    config.title = "Workbench";
    config.width = 1280;
    config.height = 720;
    config.visible = false;

    graphics::full_application_window::FullApplicationWindow window(config);

    graphics::components::MenuModel file_menu("File");
    file_menu.add_item(graphics::components::MenuItem::action("Open", "Ctrl+O"));
    graphics::components::MenuBarModel menu_bar;
    menu_bar.add_menu(file_menu);

    window.set_menu_bar(menu_bar);

    EXPECT_EQ(window.config().title, "Workbench");
    EXPECT_EQ(window.config().width, 1280U);
    EXPECT_EQ(window.config().height, 720U);
    EXPECT_FALSE(window.is_open());
    ASSERT_EQ(window.menu_bar().menus.size(), 1U);
    EXPECT_EQ(window.menu_bar().menus.front().title, "File");
}

TEST(FullApplicationWindowTest, UpdatesTitleBeforeNativeCreation) {
    graphics::full_application_window::FullApplicationWindow window;
    window.set_title("Updated Title");

    EXPECT_EQ(window.config().title, "Updated Title");
    EXPECT_FALSE(window.is_open());
}

#ifdef GRAPHICS_TEST_GTK3
TEST(FullApplicationWindowTest, GtkNativeMenusPreserveRenderingInputAndLifecycle) {
    using namespace graphics::full_application_window;
    gdk_set_allowed_backends("x11");
    if (!gtk_init_check(nullptr, nullptr) || !GDK_IS_X11_DISPLAY(gdk_display_get_default()))
        TYST_SKIP() << "Native GTK regression requires an X11 display.";
    FullApplicationWindow window(WindowConfig("CoolBox GTK regression", 480, 320));
    EXPECT_TRUE(window.has_native_menus());
    graphics::components::MenuModel edit("Edit");
    edit.add_item(graphics::components::MenuItem::action("Checked action", "Ctrl+Z"));
    edit.add_item(graphics::components::MenuItem::divider());
    edit.add_item(graphics::components::MenuItem::action("Disabled", "", false));
    graphics::components::MenuBarModel bar;
    bar.add_menu(edit);
    window.set_menu_bar(bar);
    std::vector<PointerPressEvent> presses, releases, moves;
    std::vector<std::string> typed_text;
    int commands = 0, renders = 0, keys = 0;
    window.set_menu_command_handler([&](std::size_t m, std::size_t i, const auto&, const auto&) {
        EXPECT_EQ(m, 0U);
        EXPECT_EQ(i, 0U);
        ++commands;
        EXPECT_TRUE(window.set_menu_item_checked(m, i, !window.menu_item_checked(m, i)));
    });
    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent& event) {
        EXPECT_EQ(event.native_handle, window.native_handle());
        ++renders;
        graphics::Canvas frame(static_cast<int>(event.width), static_cast<int>(event.height),
            graphics::Color{17, 91, 143});
        frame.draw_rect(20, 30, 30, 30, graphics::Color{233, 37, 51}, true);
        EXPECT_TRUE(window.present_canvas(frame));
    };
    hooks.on_key = [&](const KeyEvent&) { ++keys; };
    hooks.on_text_input = [&](const std::string& text) { typed_text.push_back(text); };
    hooks.on_pointer_press = [&](const auto& e) { presses.push_back(e); };
    hooks.on_pointer_release = [&](const auto& e) { releases.push_back(e); };
    hooks.on_pointer_move = [&](const auto& e) { moves.push_back(e); };
    window.set_render_hooks(std::move(hooks));
    ASSERT_TRUE(window.create());
    auto settle = [&]() {
        for (int i = 0; i < 20; ++i) {
            EXPECT_TRUE(window.pump_events());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };
    settle();
    GdkWindow* drawable = gdk_x11_window_lookup_for_display(gdk_display_get_default(), window.native_handle());
    ASSERT_TRUE(drawable != nullptr);
    gpointer canvas_pointer = nullptr;
    gdk_window_get_user_data(drawable, &canvas_pointer);
    GtkWidget* canvas = GTK_WIDGET(canvas_pointer);
    ASSERT_TRUE(GTK_IS_DRAWING_AREA(canvas));
    GtkWidget* host = gtk_widget_get_toplevel(canvas);
    GtkWidget* box = gtk_bin_get_child(GTK_BIN(host));
    auto child = [](GtkWidget* parent, guint index) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(parent));
        auto* result = GTK_WIDGET(g_list_nth_data(children, index));
        g_list_free(children);
        return result;
    };
    auto native_item = [&]() {
        return child(gtk_menu_item_get_submenu(GTK_MENU_ITEM(child(child(box, 0), 0))), 0);
    };
    GtkWidget* menu = child(box, 0);
    ASSERT_TRUE(GTK_IS_MENU_BAR(menu));
    EXPECT_EQ(child(box, 1), canvas);
    GtkWidget* popup = gtk_menu_item_get_submenu(GTK_MENU_ITEM(child(menu, 0)));
    EXPECT_FALSE(gtk_widget_get_sensitive(child(popup, 2)));
    gtk_menu_item_activate(GTK_MENU_ITEM(child(popup, 2)));
    EXPECT_EQ(commands, 0);
    gtk_menu_item_activate(GTK_MENU_ITEM(native_item()));
    EXPECT_EQ(commands, 1);
    EXPECT_TRUE(window.menu_item_checked(0, 0));
    EXPECT_TRUE(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(native_item())));
    EXPECT_TRUE(gtk_accel_groups_activate(G_OBJECT(host), GDK_KEY_z, GDK_CONTROL_MASK));
    EXPECT_EQ(commands, 2);
    EXPECT_FALSE(window.menu_item_checked(0, 0));
    EXPECT_EQ(keys, 0);
    settle();
    Display* display = GDK_WINDOW_XDISPLAY(drawable);
    XSync(display, False);
    XWindowAttributes attributes{};
    ASSERT_TRUE(XGetWindowAttributes(display, window.native_handle(), &attributes));
    int width = 0, height = 0;
    ASSERT_TRUE(window.client_size(width, height));
    EXPECT_EQ(width, attributes.width);
    EXPECT_EQ(height, attributes.height);
    EXPECT_EQ(width, static_cast<int>(window.config().width));
    EXPECT_EQ(height, static_cast<int>(window.config().height));
    XImage* image = XGetImage(display, window.native_handle(), 0, 0, 80, 80, AllPlanes, ZPixmap);
    ASSERT_TRUE(image != nullptr);
    EXPECT_GT(renders, 0);
    EXPECT_NE(XGetPixel(image, 5, 5), XGetPixel(image, 25, 35));
    XDestroyImage(image);
    gtk_window_present(GTK_WINDOW(host));
    settle();
    PointerState state{};
    ASSERT_TRUE(window.query_pointer_state(state));
    EXPECT_EQ(state.focused, gtk_window_is_active(GTK_WINDOW(host)) != FALSE);
    EXPECT_TRUE(state.focused);
    auto* device = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default()));
    auto button = [&](GdkEventType type, guint number, double x, double y) {
        GdkEvent* event = gdk_event_new(type);
        event->button.window = GDK_WINDOW(g_object_ref(drawable));
        event->button.send_event = TRUE;
        event->button.time = GDK_CURRENT_TIME;
        event->button.x = x;
        event->button.y = y;
        event->button.button = number;
        gdk_event_set_device(event, device);
        gdk_event_put(event);
        gdk_event_free(event);
    };
    button(GDK_BUTTON_PRESS, 1, 40, 50);
    button(GDK_BUTTON_RELEASE, 1, -10, height + 20);
    button(GDK_BUTTON_PRESS, 3, 60, 70);
    button(GDK_BUTTON_RELEASE, 3, 60, 70);
    GdkEvent* motion = gdk_event_new(GDK_MOTION_NOTIFY);
    motion->motion.window = GDK_WINDOW(g_object_ref(drawable));
    motion->motion.x = 80;
    motion->motion.y = 90;
    gdk_event_set_device(motion, device);
    gdk_event_put(motion);
    gdk_event_free(motion);
    settle();
    ASSERT_EQ(presses.size(), 2U);
    ASSERT_EQ(releases.size(), 2U);
    EXPECT_FALSE(moves.empty());
    EXPECT_EQ(presses[0].button, PointerButton::Left);
    EXPECT_EQ(releases[0].x, -10);
    EXPECT_EQ(releases[0].y, height + 20);
    EXPECT_EQ(presses[1].button, PointerButton::Right);
    EXPECT_EQ(releases[1].button, PointerButton::Right);
    GdkEventKey typing{};
    typing.type = GDK_KEY_PRESS;
    typing.window = drawable;
    typing.keyval = GDK_KEY_A;
    typing.state = GDK_SHIFT_MASK;
    gboolean handled = FALSE;
    g_signal_emit_by_name(host, "key-press-event", &typing, &handled);
    typing.keyval = GDK_KEY_eacute;
    typing.state = 0;
    g_signal_emit_by_name(host, "key-press-event", &typing, &handled);
    typing.keyval = GDK_KEY_Return;
    g_signal_emit_by_name(host, "key-press-event", &typing, &handled);
    ASSERT_EQ(typed_text.size(), 2U);
    EXPECT_EQ(typed_text[0], "A");
    EXPECT_EQ(typed_text[1], std::string("\xc3\xa9"));
    window.set_menu_bar({});
    settle();
    EXPECT_EQ(child(box, 0), canvas);
    window.set_menu_bar(bar);
    settle();
    window.close();
    EXPECT_FALSE(window.is_open());
    ASSERT_TRUE(window.create());
    settle();
    window.close();
}
#endif

#ifdef _WIN32
TEST(FullApplicationWindowTest, DeliversQuickClicksAndNativeMenuCommandsFromMessageQueue) {
    using namespace graphics::full_application_window;
    FullApplicationWindow window(WindowConfig("Native input", 400, 300, false, true));
    std::vector<PointerPressEvent> clicks;
    std::vector<PointerPressEvent> releases;
    std::vector<PointerPressEvent> moves;
    std::vector<std::pair<std::size_t, std::size_t>> commands;
    RenderHooks hooks;
    hooks.on_pointer_press = [&](const PointerPressEvent& event) { clicks.push_back(event); };
    hooks.on_pointer_release = [&](const PointerPressEvent& event) { releases.push_back(event); };
    hooks.on_pointer_move = [&](const PointerPressEvent& event) { moves.push_back(event); };
    window.set_render_hooks(std::move(hooks));
    graphics::components::MenuModel edit("&Edit");
    edit.add_item(graphics::components::MenuItem::action("Undo", "Ctrl+Z"));
    edit.add_item(graphics::components::MenuItem::divider());
    edit.add_item(graphics::components::MenuItem::action("Disabled", "", false));
    graphics::components::MenuBarModel bar;
    bar.add_menu(edit);
    window.set_menu_bar(bar);
    window.set_menu_command_handler([&](std::size_t menu, std::size_t item, const auto&, const auto&) {
        commands.emplace_back(menu, item);
    });
    ASSERT_TRUE(window.create());
    const auto handle = reinterpret_cast<HWND>(window.native_handle());
    const auto native_bar = GetMenu(handle);
    ASSERT_TRUE(native_bar != nullptr);
    const auto popup = GetSubMenu(native_bar, 0);
    ASSERT_EQ(GetMenuItemCount(popup), 3);
    PostMessage(handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(40, 60));
    PostMessage(handle, WM_LBUTTONUP, 0, MAKELPARAM(40, 60));
    PostMessage(handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(80, 90));
    PostMessage(handle, WM_LBUTTONUP, 0, MAKELPARAM(80, 90));
    PostMessage(handle, WM_MOUSEMOVE, 0, MAKELPARAM(100, 120));
    PostMessage(handle, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(110, 130));
    PostMessage(handle, WM_RBUTTONUP, 0, MAKELPARAM(110, 130));
    PostMessage(handle, WM_COMMAND, GetMenuItemID(popup, 0), 0);
    PostMessage(handle, WM_COMMAND, GetMenuItemID(popup, 2), 0);
    ASSERT_TRUE(window.pump_events());
    ASSERT_EQ(clicks.size(), 3U);
    EXPECT_EQ(clicks[0].x, 40);
    EXPECT_EQ(clicks[0].y, 60);
    EXPECT_EQ(clicks[1].x, 80);
    EXPECT_EQ(clicks[2].button, PointerButton::Right);
    ASSERT_EQ(releases.size(), 3U);
    EXPECT_EQ(releases[0].button, PointerButton::Left);
    EXPECT_EQ(releases[2].button, PointerButton::Right);
    ASSERT_FALSE(moves.empty());
    EXPECT_EQ(moves.back().x, 100);
    EXPECT_EQ(moves.back().y, 120);
    ASSERT_EQ(commands.size(), 1U);
    EXPECT_EQ(commands[0].first, 0U);
    EXPECT_EQ(commands[0].second, 0U);
    char label[64]{};
    GetMenuStringA(popup, 0, label, sizeof(label), MF_BYPOSITION);
    EXPECT_EQ(std::string(label), "Undo\tCtrl+Z");
    window.close();
    MSG quit{};
    PeekMessage(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
}

TEST(FullApplicationWindowTest, TranslatesKeyboardNavigationAndRepeatOnWindows) {
    using namespace graphics::full_application_window;
    FullApplicationWindow window(WindowConfig("Keyboard events", 400, 300, false, true));
    std::vector<KeyEvent> keys;
    std::vector<std::string> text;
    RenderHooks hooks;
    hooks.on_key = [&](const KeyEvent& event) { keys.push_back(event); };
    hooks.on_text_input = [&](const std::string& value) { text.push_back(value); };
    window.set_render_hooks(std::move(hooks));
    ASSERT_TRUE(window.create());
    const auto handle = reinterpret_cast<HWND>(window.native_handle());
    SendMessage(handle, WM_KEYDOWN, VK_RIGHT, 1);
    SendMessage(handle, WM_KEYDOWN, VK_SPACE, (LPARAM{1} << 30) | 1);
    SendMessage(handle, WM_KEYDOWN, VK_DELETE, 1);
    ASSERT_EQ(keys.size(), 3U);
    EXPECT_EQ(keys[0].key, static_cast<int>(Key::Right));
    EXPECT_FALSE(keys[0].repeat);
    EXPECT_EQ(keys[1].key, static_cast<int>(Key::Space));
    EXPECT_TRUE(keys[1].repeat);
    EXPECT_EQ(keys[2].key, static_cast<int>(Key::Delete));
    SendMessage(handle, WM_CHAR, 'a', 1);
    SendMessage(handle, WM_CHAR, 0x00e9, 1);
    SendMessage(handle, WM_CHAR, 0xd83d, 1);
    SendMessage(handle, WM_CHAR, 0xdcf9, 1);
    SendMessage(handle, WM_CHAR, 13, 1);
    ASSERT_EQ(text.size(), 3U);
    EXPECT_EQ(text[0], "a");
    EXPECT_EQ(text[1], std::string("\xc3\xa9"));
    EXPECT_EQ(text[2], std::string("\xf0\x9f\x93\xb9"));
    window.close();
    MSG quit{};
    PeekMessage(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
}

TEST(FullApplicationWindowTest, EnforcesMinimumClientSizeOnWindows) {
    graphics::full_application_window::WindowConfig config("Minimum size", 1280, 800, false, true);
    config.min_width = 960;
    config.min_height = 640;
    graphics::full_application_window::FullApplicationWindow window(config);
    ASSERT_TRUE(window.create());
    MINMAXINFO sizes{};
    SendMessage(reinterpret_cast<HWND>(window.native_handle()), WM_GETMINMAXINFO, 0,
                reinterpret_cast<LPARAM>(&sizes));
    EXPECT_GT(sizes.ptMinTrackSize.x, 960);
    EXPECT_GT(sizes.ptMinTrackSize.y, 640);
    window.close();
    // This fixture owns a native window; don't leave its quit message for the next test.
    MSG quit{};
    PeekMessage(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
}
#endif

TEST(FullApplicationWindowTest, StoresAndInvokesHeadlessRenderHooks) {
    graphics::full_application_window::WindowConfig config;
    config.visible = false;

    graphics::full_application_window::FullApplicationWindow window(config);
    std::size_t create_calls = 0;
    std::size_t render_calls = 0;
    std::size_t tick_calls = 0;

    graphics::full_application_window::RenderHooks hooks;
    hooks.on_create = [&](const graphics::full_application_window::RenderEvent&) { ++create_calls; };
    hooks.on_render = [&](const graphics::full_application_window::RenderEvent&) { ++render_calls; };
    hooks.on_tick = [&](const graphics::full_application_window::RenderEvent&) { ++tick_calls; };
    window.set_render_hooks(hooks);

    ASSERT_TRUE(window.create());
    window.request_redraw();
    ASSERT_TRUE(window.pump_events());

    EXPECT_GE(create_calls, 1U);
    EXPECT_GE(tick_calls, 1U);
    EXPECT_TRUE(static_cast<bool>(window.render_hooks().on_render));

    if (window.backend() == graphics::full_application_window::Backend::Headless) {
        EXPECT_GE(render_calls, 1U);
    }
}

TEST(FullApplicationWindowTest, SharedBaseContractCoversWindowModelsAndHosts) {
    graphics::full_application_window::WindowConfig config("Workbench", 1280, 720, false, true);
    graphics::full_application_window::RenderEvent event;
    event.frame_index = 7;

    graphics::full_application_window::WorkspacePanelLayout layout(240, 320, 180, 6);
    graphics::full_application_window::WorkspaceNavItem nav_item("[W]", "Workspace");
    graphics::full_application_window::WorkspaceDockModels dock_models(
        "Editor",
        "content",
        {nav_item},
        0,
        "Files",
        {"main.cpp"},
        0,
        "Inspector",
        "Preview",
        "Ready",
        layout);
    graphics::full_application_window::FullApplicationWindow window(config);
    graphics::full_application_window::WorkspaceDockHost host;

    assert_graphics_object_contract(config, "windowConfig", "Workbench");
    assert_graphics_object_contract(event, "renderEvent", "frame-7");
    assert_graphics_object_contract(layout, "workspacePanelLayout", "240x320x180");
    assert_graphics_object_contract(nav_item, "workspaceNavItem", "Workspace");
    assert_graphics_object_contract(dock_models, "workspaceDockModels", "Editor");
    assert_graphics_object_contract(window, "fullApplicationWindow", "Workbench");

    EXPECT_EQ(host.graphics_object_kind(), "workspaceDockHost");
    EXPECT_FALSE(host.graphics_object_name().empty());
}

TEST(FullApplicationWindowTest, RegistryCreatesFullApplicationObjectsPolymorphically) {
    graphics::GraphicsObjectRegistry registry;
    ASSERT_TRUE(registry.register_type<graphics::full_application_window::RenderEvent>("renderEvent"));
    ASSERT_TRUE(registry.register_factory("window", []() {
        return std::make_unique<graphics::full_application_window::FullApplicationWindow>(
            graphics::full_application_window::WindowConfig("Registry Window", 800, 600, false, true));
    }));
    ASSERT_TRUE(registry.register_factory("dockModels", []() {
        return std::make_unique<graphics::full_application_window::WorkspaceDockModels>(
            "Registry Dock",
            "editor text",
            std::vector<graphics::full_application_window::WorkspaceNavItem>{{"[F]", "Files"}},
            0,
            "Browser",
            std::vector<std::string>{"readme.md"},
            0,
            "Inspector",
            "Preview",
            "Ready",
            graphics::full_application_window::WorkspacePanelLayout(260, 300, 180, 6));
    }));

    EXPECT_TRUE(registry.contains("window"));
    EXPECT_GE(registry.registered_keys().size(), 3U);

    const auto event = registry.create("renderEvent");
    const auto window = registry.create("window");
    const auto dock_models = registry.create("dockModels");

    assert_graphics_object_contract(*event, "renderEvent", "frame-0");
    assert_graphics_object_contract(*window, "fullApplicationWindow", "Registry Window");
    assert_graphics_object_contract(*dock_models, "workspaceDockModels", "Registry Dock");
}