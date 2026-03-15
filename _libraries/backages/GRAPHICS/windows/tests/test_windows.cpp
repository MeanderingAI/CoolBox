#include <gtest/gtest.h>

#include <filesystem>

#include "graphics.h"
#include "json.h"
#include "windows.hpp"

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
