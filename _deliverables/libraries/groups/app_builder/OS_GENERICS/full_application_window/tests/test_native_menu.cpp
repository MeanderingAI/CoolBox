#include "native_menu.hpp"
#include "full_application_window.hpp"
#include "tyst_framework.hpp"

#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#ifdef GRAPHICS_TEST_GTK3
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

using namespace graphics::os_generic;
using graphics::full_application_window::FullApplicationWindow;

namespace {
void select(FullApplicationWindow& window, std::size_t menu, std::size_t item) {
    const auto model = window.menu_bar();
    const auto handler = window.menu_command_handler();
    handler(menu, item, model.menus.at(menu), model.menus.at(menu).items.at(item));
}
}

TEST(NativeMenuApi, InstallsCommandsAndUpdatesStateById) {
    FullApplicationWindow window;
    NativeMenuBar bar(window);
    if (!bar.supported()) TYST_SKIP() << "This backend has no native menus.";
    int opens = 0, undos = 0;
    bar.set_menus({
        {"&File", {{"file.open", "Open", "Ctrl+O", [&] { ++opens; }},
                    MenuAction::divider(), {"file.save", "Save", "Ctrl+S", [] {}, false}}},
        {"&Edit", {{"edit.undo", "Undo", "Ctrl+Z", [&] { ++undos; }}}}
    });
    ASSERT_EQ(window.menu_bar().menus.size(), 2U);
    EXPECT_EQ(window.menu_bar().menus[0].items[0].shortcut, "Ctrl+O");
    select(window, 0, 0);
    EXPECT_EQ(opens, 1);
    bar.set_enabled("file.open", false);
    EXPECT_FALSE(bar.enabled("file.open"));
    EXPECT_FALSE(window.menu_bar().menus[0].items[0].enabled);
    select(window, 0, 0);
    EXPECT_EQ(opens, 1);
    select(window, 0, 1);
    EXPECT_EQ(opens, 1);
    bar.set_checked("edit.undo", true);
    EXPECT_TRUE(bar.checked("edit.undo"));
    EXPECT_TRUE(window.menu_bar().menus[1].items[0].checked);
    select(window, 1, 0);
    EXPECT_EQ(undos, 1);
    EXPECT_THROW(bar.set_enabled("missing", true), std::invalid_argument);
    EXPECT_THROW(bar.checked("missing"), std::invalid_argument);
    bar.set_menus({});
    EXPECT_TRUE(window.menu_bar().menus.empty());
}

TEST(NativeMenuApi, InvalidDefinitionsPreserveInstalledMenu) {
    FullApplicationWindow window;
    NativeMenuBar bar(window);
    if (!bar.supported()) TYST_SKIP() << "This backend has no native menus.";
    bar.set_menus({{"File", {{"open", "Open", "", [] {}}}}});
    EXPECT_THROW(bar.set_menus({{"Edit", {{"same", "One", "", [] {}},
        {"same", "Two", "", [] {}}}}}), std::invalid_argument);
    EXPECT_THROW(bar.set_menus({{"", {{"open", "Open", "", [] {}}}}}), std::invalid_argument);
    EXPECT_THROW(bar.set_menus({{"File", {{"", "Open", "", [] {}}}}}), std::invalid_argument);
    EXPECT_THROW(bar.set_menus({{"File", {{"open", "", "", [] {}}}}}), std::invalid_argument);
    EXPECT_THROW(bar.set_menus({{"File", {{"open", "Open", "", {}}}}}), std::invalid_argument);
    EXPECT_EQ(window.menu_bar().menus[0].title, "File");
}

TEST(NativeMenuApi, CallbackCanReplaceItsOwnMenuAndOutliveController) {
    FullApplicationWindow window;
    int calls = 0;
    {
        NativeMenuBar bar(window);
        if (!bar.supported()) TYST_SKIP() << "This backend has no native menus.";
        bar.set_menus({{"File", {{"replace", "Replace", "", [&] {
            ++calls;
            bar.set_menus({{"Edit", {{"next", "Next", "", [&] { ++calls; }}}}});
        }}}}});
        select(window, 0, 0);
        EXPECT_EQ(calls, 1);
        EXPECT_EQ(window.menu_bar().menus[0].title, "Edit");
    }
    select(window, 0, 0);
    EXPECT_EQ(calls, 2);
}

#ifdef _WIN32
TEST(NativeMenuApi, DispatchesThroughRealWin32MenuAndRebuildsAfterCreation) {
    FullApplicationWindow window(graphics::full_application_window::WindowConfig("Native menu API", 400, 300, false));
    NativeMenuBar bar(window);
    int calls = 0;
    bar.set_menus({{"&File", {{"open", "Open", "Ctrl+O", [&] { ++calls; }}}}});
    ASSERT_TRUE(window.create());
    const auto hwnd = reinterpret_cast<HWND>(window.native_handle());
    auto click = [&] {
        const auto popup = GetSubMenu(GetMenu(hwnd), 0);
        SendMessage(hwnd, WM_COMMAND, GetMenuItemID(popup, 0), 0);
    };
    click();
    EXPECT_EQ(calls, 1);
    bar.set_enabled("open", false);
    click();
    EXPECT_EQ(calls, 1);
    bar.set_enabled("open", true);
    bar.set_checked("open", true);
    EXPECT_NE(GetMenuState(GetSubMenu(GetMenu(hwnd), 0), 0, MF_BYPOSITION) & MF_CHECKED, 0U);
    click();
    EXPECT_EQ(calls, 2);
    window.close();
    MSG quit{};
    PeekMessage(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
}
#endif

#ifdef GRAPHICS_TEST_GTK3
TEST(NativeMenuApi, DispatchesThroughRealGtkMenuAndRebuildsAfterCreation) {
    gdk_set_allowed_backends("x11");
    if (!gtk_init_check(nullptr, nullptr)) TYST_SKIP() << "An X11 display is required.";
    FullApplicationWindow window;
    NativeMenuBar bar(window);
    int calls = 0;
    bar.set_menus({{"&File", {{"open", "Open", "Ctrl+O", [&] { ++calls; }}}}});
    ASSERT_TRUE(window.create());
    auto* drawable = gdk_x11_window_lookup_for_display(gdk_display_get_default(), window.native_handle());
    ASSERT_TRUE(drawable != nullptr);
    gpointer canvas = nullptr;
    gdk_window_get_user_data(drawable, &canvas);
    auto* host = gtk_widget_get_toplevel(GTK_WIDGET(canvas));
    auto child = [](GtkWidget* parent) {
        auto* children = gtk_container_get_children(GTK_CONTAINER(parent));
        auto* result = GTK_WIDGET(children->data);
        g_list_free(children);
        return result;
    };
    auto item = [&] {
        auto* menu_bar = child(gtk_bin_get_child(GTK_BIN(host)));
        return child(gtk_menu_item_get_submenu(GTK_MENU_ITEM(child(menu_bar))));
    };
    gtk_menu_item_activate(GTK_MENU_ITEM(item()));
    EXPECT_EQ(calls, 1);
    bar.set_enabled("open", false);
    EXPECT_FALSE(gtk_widget_get_sensitive(item()));
    gtk_menu_item_activate(GTK_MENU_ITEM(item()));
    EXPECT_EQ(calls, 1);
    bar.set_enabled("open", true);
    bar.set_checked("open", true);
    EXPECT_TRUE(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item())));
    EXPECT_TRUE(gtk_accel_groups_activate(G_OBJECT(host), GDK_KEY_o, GDK_CONTROL_MASK));
    EXPECT_EQ(calls, 2);
    window.close();
}
#endif
