#include "tyst_framework.hpp"

#include <filesystem>
#include <type_traits>

#include "components.hpp"
#include "graphics.h"
#include "json.h"
#include "windows.hpp"

namespace {

void assert_graphics_object_contract(const graphics::GraphicsObject& object,
                                     const std::string& expected_kind,
                                     const std::string& expected_name) {
    EXPECT_EQ(object.graphics_object_kind(), expected_kind);
    EXPECT_EQ(object.graphics_object_name(), expected_name);
}

static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::windows::MenuItem>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::windows::Menu>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::windows::CadViewport>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::windows::Panel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::windows::WindowSimulator>);

} // namespace

TEST(WindowSimulatorTest, RendersPlatformSpecificTitleBarAndMenus) {
    graphics::windows::WindowSimulator window("CoolBox", 60, 12, graphics::windows::PlatformStyle::MacOS);
    graphics::windows::Menu file_menu("File");
    file_menu
        .add_item(graphics::windows::MenuItem::action("New", "Ctrl+N"))
        .add_item(graphics::windows::MenuItem::action("Open", "Ctrl+O"))
        .add_item(graphics::windows::MenuItem::divider())
        .add_item(graphics::windows::MenuItem::action("Quit", "Ctrl+Q"));

    window.add_menu(file_menu)
          .set_content({"Welcome to CoolBox", "Cross-platform window simulation"})
          .set_status_text("Ready");

    const std::string rendered = window.render();
    EXPECT_NE(rendered.find("CoolBox [macOS]"), std::string::npos);
    EXPECT_NE(rendered.find("File"), std::string::npos);
    EXPECT_NE(rendered.find("Welcome to CoolBox"), std::string::npos);
}

TEST(WindowSimulatorTest, RendersDropdownMenusWithShortcuts) {
    graphics::windows::WindowSimulator window("CoolBox", 50, 10, graphics::windows::PlatformStyle::Windows);
    graphics::windows::Menu file_menu("File");
    file_menu
        .add_item(graphics::windows::MenuItem::action("Open", "Ctrl+O"))
        .add_item(graphics::windows::MenuItem::action("Save", "Ctrl+S", true, true))
        .add_item(graphics::windows::MenuItem::divider())
        .add_item(graphics::windows::MenuItem::action("Exit", "Alt+F4", false));

    window.add_menu(file_menu);
    const std::string dropdown = window.render_menu_dropdown("File");

    EXPECT_NE(dropdown.find("Open"), std::string::npos);
    EXPECT_NE(dropdown.find("Ctrl+O"), std::string::npos);
    EXPECT_NE(dropdown.find("✓ Save"), std::string::npos);
    EXPECT_NE(dropdown.find("Alt+F4"), std::string::npos);
}

TEST(WindowSimulatorTest, ThrowsForUnknownMenu) {
    graphics::windows::WindowSimulator window;
    EXPECT_THROW(window.render_menu_dropdown("Missing"), std::invalid_argument);
}

TEST(WindowSimulatorTest, RendersGenericChartAndCadPanels) {
    graphics::Graph graph(320, 200, graphics::GraphType::Line);
    graph.set_title("Load Profile");
    graph.set_x_label("Time");
    graph.set_y_label("Torque");
    graph.add_series({"Torque", {0.0, 1.0, 2.0, 3.0}, {42.0, 48.0, 65.0, 82.4}, graphics::Colors::Blue});

    graphics::windows::CadViewport viewport("Assembly Preview");
    viewport.projection = "Orthographic";
    viewport.layers = {"Frame", "Fasteners", "Annotations"};
    viewport.primitive_count = 128;

    graphics::windows::WindowSimulator window("Design Studio", 72, 28, graphics::windows::PlatformStyle::Linux);
    window.add_menu(graphics::windows::Menu("File").add_item(graphics::windows::MenuItem::action("Export", "Ctrl+E")))
          .add_panel(graphics::windows::Panel::generic("Inspector",
                                                       {"Selection: Gear Housing", "Material: Aluminum", "Revision: B"},
                                                       5))
          .add_panel(graphics::windows::Panel::chart_preview("Load Profile", graph, {"Series: Torque vs Time", "Last sample: 82.4"}))
          .add_panel(graphics::windows::Panel::cad_viewport(viewport, 7))
          .set_status_text("Panels active");

    const std::string rendered = window.render();
    EXPECT_NE(rendered.find("Inspector"), std::string::npos);
    EXPECT_NE(rendered.find("[Chart] Load Profile"), std::string::npos);
    EXPECT_NE(rendered.find("Canvas: 320x200"), std::string::npos);
    EXPECT_NE(rendered.find("Embedded chart preview:"), std::string::npos);
    EXPECT_NE(rendered.find("Source: graphics::Graph"), std::string::npos);
    EXPECT_NE(rendered.find("[3D CAD Linux] Assembly Preview"), std::string::npos);
    EXPECT_NE(rendered.find("Projection: Orthographic"), std::string::npos);
    EXPECT_NE(rendered.find("Panels active"), std::string::npos);
}

TEST(WindowSimulatorTest, ExportsWindowLayoutToJson) {
    graphics::Canvas canvas(120, 80);
    graphics::windows::CadViewport viewport("Part Studio");
    viewport.layers = {"Bodies", "Sketches"};
    viewport.primitive_count = 24;

    graphics::windows::WindowSimulator window("Workbench", 64, 20, graphics::windows::PlatformStyle::Windows);
    window.add_menu(graphics::windows::Menu("File").add_item(graphics::windows::MenuItem::action("Save", "Ctrl+S")))
          .add_panel(graphics::windows::Panel::chart_preview("Metrics", canvas, {"Series: Stress"}))
          .add_panel(graphics::windows::Panel::cad_viewport(viewport, 6))
          .set_status_text("Export ready");

    const std::string json = window.to_json();
    const auto parsed = dataformats::json::Parser::parse(json).as_object();
    EXPECT_EQ(parsed.get("title").as_string(), "Workbench");
    EXPECT_EQ(parsed.get("platform").as_string(), "Windows");

    const auto panels = parsed.get("panels").as_array();
    ASSERT_EQ(panels.size(), 2U);
    EXPECT_EQ(panels.get(0).as_object().get("kind").as_string(), "chart");
    EXPECT_EQ(static_cast<int>(panels.get(0).as_object().get("embeddedChart").as_object().get("width").as_number()), 120);
    EXPECT_EQ(static_cast<int>(panels.get(1).as_object().get("cadViewport").as_object().get("primitiveCount").as_number()), 24);

    const std::filesystem::path export_path = std::filesystem::current_path() / "window_layout_export.json";
    ASSERT_TRUE(window.save_json(export_path.string()));
    ASSERT_TRUE(std::filesystem::exists(export_path));

    std::filesystem::remove(export_path);
}

TEST(WindowSimulatorTest, EmbedsGraphicsComponentsInsideWindowsPanels) {
    const std::vector<graphics::components::Component> widgets = {
        graphics::components::Component::button("Apply", true, false, 12),
        graphics::components::Component::text_view("Status text appears in a compact panel.", 20),
        graphics::components::Component::editable_text_view("Notes", 5, true, 16),
        graphics::components::Component::radio_button("Metric Units", true),
        graphics::components::Component::check_box("Enable Grid", true)
    };

    graphics::windows::WindowSimulator window("UI Designer", 74, 26, graphics::windows::PlatformStyle::MacOS);
    window.add_panel(graphics::windows::Panel::component_group("Controls", widgets))
          .set_status_text("Component panel ready");

    const std::string rendered = window.render();
    EXPECT_NE(rendered.find("[Components] Controls"), std::string::npos);
    EXPECT_NE(rendered.find("Apply"), std::string::npos);
    EXPECT_NE(rendered.find("Notes|"), std::string::npos);
    EXPECT_NE(rendered.find("(o) Metric Units"), std::string::npos);
    EXPECT_NE(rendered.find("[x] Enable Grid"), std::string::npos);
}

TEST(WindowSimulatorTest, ExportsEmbeddedComponentsToJson) {
    const std::vector<graphics::components::Component> widgets = {
        graphics::components::Component::button("Save", true, true, 10),
        graphics::components::Component::check_box("Autosave", false)
    };

    graphics::windows::WindowSimulator window("Component Export", 68, 22, graphics::windows::PlatformStyle::Linux);
    window.add_panel(graphics::windows::Panel::component_group("Toolbar", widgets));

    const auto parsed = dataformats::json::Parser::parse(window.to_json()).as_object();
    const auto panels = parsed.get("panels").as_array();
    ASSERT_EQ(panels.size(), 1U);

    const auto panel = panels.get(0).as_object();
    EXPECT_EQ(panel.get("kind").as_string(), "componentGroup");

    const auto components = panel.get("components").as_array();
    ASSERT_EQ(components.size(), 2U);
    EXPECT_EQ(components.get(0).as_object().get("type").as_string(), "button");
    EXPECT_EQ(components.get(0).as_object().get("pressed").as_bool(), true);
    EXPECT_EQ(components.get(1).as_object().get("type").as_string(), "checkBox");
}

TEST(WindowSimulatorTest, EmbedsComponentHolderLayoutsInWindowsPanels) {
    const auto nested_toolbar = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::button("Open", true, false, 10),
        graphics::components::Component::button("Close", true, false, 10)
    }, 2);

    const auto holder = graphics::components::ComponentHolder::grid({
        graphics::components::Component::layout_group(nested_toolbar, "Toolbar"),
        graphics::components::Component::editable_text_view("Draft", 5, true, 12),
        graphics::components::Component::check_box("Autosave", true),
        graphics::components::Component::radio_button("Sync", true)
    }, 2, 3, 1);

    graphics::windows::WindowSimulator window("Layout Studio", 78, 24, graphics::windows::PlatformStyle::Windows);
    window.add_panel(graphics::windows::Panel::component_group("Grid Controls", holder))
          .set_status_text("Holder embedded");

    const std::string rendered = window.render();
    EXPECT_NE(rendered.find("[Components] Grid Controls"), std::string::npos);
    EXPECT_NE(rendered.find("Toolbar"), std::string::npos);
    EXPECT_NE(rendered.find("Open"), std::string::npos);
    EXPECT_NE(rendered.find("Draft|"), std::string::npos);

    const auto parsed = dataformats::json::Parser::parse(window.to_json()).as_object();
    const auto panel = parsed.get("panels").as_array().get(0).as_object();
    EXPECT_EQ(panel.get("componentLayout").as_string(), "grid");
    EXPECT_EQ(static_cast<int>(panel.get("componentColumns").as_number()), 2);
    const auto components = panel.get("components").as_array();
    EXPECT_EQ(components.get(0).as_object().get("type").as_string(), "layoutGroup");
    EXPECT_EQ(components.get(0).as_object().get("layoutGroup").as_object().get("layout").as_string(), "horizontal");
}

TEST(WindowSimulatorTest, SharedBaseContractCoversWindowModels) {
    graphics::windows::MenuItem item = graphics::windows::MenuItem::action("Save", "Ctrl+S");
    graphics::windows::Menu menu("File");
    menu.add_item(item);

    graphics::windows::CadViewport viewport("Assembly");
    graphics::windows::Panel panel = graphics::windows::Panel::generic("Inspector", {"Selection"}, 4);
    graphics::windows::WindowSimulator window("Workbench", 64, 20, graphics::windows::PlatformStyle::Windows);

    assert_graphics_object_contract(item, "windowMenuItem", "Save");
    assert_graphics_object_contract(menu, "windowMenu", "File");
    assert_graphics_object_contract(viewport, "cadViewport", "Assembly");
    assert_graphics_object_contract(panel, "panel", "Inspector");
    assert_graphics_object_contract(window, "windowSimulator", "Workbench");
}

TEST(WindowSimulatorTest, RegistryCreatesWindowObjectsPolymorphically) {
    graphics::GraphicsObjectRegistry registry;
    ASSERT_TRUE(registry.register_type<graphics::windows::Menu>("menu"));
    ASSERT_TRUE(registry.register_factory("chartPanel", []() {
        return std::make_unique<graphics::windows::Panel>(
            graphics::windows::Panel::generic("Registry Panel", {"Line 1"}, 4));
    }));
    ASSERT_TRUE(registry.register_factory("window", []() {
        return std::make_unique<graphics::windows::WindowSimulator>(
            graphics::windows::WindowSimulator("Registry Window", 70, 24, graphics::windows::PlatformStyle::Linux));
    }));

    EXPECT_TRUE(registry.contains("menu"));
    EXPECT_GE(registry.registered_keys().size(), 3U);

    const auto menu = registry.create("menu");
    const auto panel = registry.create("chartPanel");
    const auto window = registry.create("window");

    assert_graphics_object_contract(*menu, "windowMenu", "");
    assert_graphics_object_contract(*panel, "panel", "Registry Panel");
    assert_graphics_object_contract(*window, "windowSimulator", "Registry Window");
}
