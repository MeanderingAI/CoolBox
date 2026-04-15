#include "tyst_framework.hpp"

#include <type_traits>

#include "full_application_window.hpp"
#include "workspace_dock_host.hpp"

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