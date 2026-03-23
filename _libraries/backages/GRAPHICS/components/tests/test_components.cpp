#include <gtest/gtest.h>

#include "components.hpp"

TEST(GraphicsComponentsTest, RendersToolbarDockPanelLayerListPropertyInspectorFileTreeRadioCheckboxGroup) {
    using namespace graphics::components;
    // Toolbar
    ToolbarModel toolbar({"New", "Open", "Save"}, 1);
    auto toolbar_lines = Component::toolbar(toolbar).render();
    ASSERT_EQ(toolbar_lines.size(), 1U);
    EXPECT_NE(toolbar_lines.front().find("{New}"), std::string::npos);
    EXPECT_NE(toolbar_lines.front().find("{Open}"), std::string::npos);
    EXPECT_NE(toolbar_lines.front().find("{Save}"), std::string::npos);

    // DockPanel
    DockPanelModel dock_panel("Layers", true);
    auto dock_panel_lines = Component::dock_panel(dock_panel).render();
    ASSERT_EQ(dock_panel_lines.size(), 1U);
    EXPECT_NE(dock_panel_lines.front().find("[DockPanel] Layers [floating]"), std::string::npos);

    // LayerList
    LayerListModel layers({"Background", "Sketch", "Ink"}, 1);
    auto layer_lines = Component::layer_list(layers).render();
    ASSERT_GE(layer_lines.size(), 2U);
    EXPECT_NE(layer_lines[1].find("> Sketch"), std::string::npos);

    // PropertyInspector
    PropertyInspectorModel props;
    props.add_property("Color", "Black").add_property("Width", "2px");
    auto prop_lines = Component::property_inspector(props).render();
    ASSERT_GE(prop_lines.size(), 2U);
    EXPECT_NE(prop_lines[1].find("Color: Black"), std::string::npos);

    // FileTree
    FileTreeModel::Node root{"root", true, {
        {"file1.txt", false, {}},
        {"dir1", true, {{"file2.txt", false, {}}}}
    }};
    FileTreeModel file_tree(root);
    auto file_tree_lines = Component::file_tree(file_tree).render();
    ASSERT_GE(file_tree_lines.size(), 3U);
    EXPECT_NE(file_tree_lines[0].find("[D] root"), std::string::npos);
    EXPECT_NE(file_tree_lines[1].find("file1.txt"), std::string::npos);
    EXPECT_NE(file_tree_lines[2].find("[D] dir1"), std::string::npos);

    // RadioSelector
    RadioSelectorModel radios({"A", "B", "C"}, 2);
    auto radio_lines = Component::radio_selector(radios).render();
    ASSERT_EQ(radio_lines.size(), 3U);
    EXPECT_EQ(radio_lines[2], "(o) C");

    // CheckboxGroup
    CheckboxGroupModel checks({"X", "Y", "Z"}, {true, false, true});
    auto check_lines = Component::checkbox_group(checks).render();
    ASSERT_EQ(check_lines.size(), 3U);
    EXPECT_EQ(check_lines[0], "[x] X");
    EXPECT_EQ(check_lines[1], "[ ] Y");
    EXPECT_EQ(check_lines[2], "[x] Z");
}
#include <gtest/gtest.h>

#include "components.hpp"

TEST(GraphicsComponentsTest, RendersBasicWidgets) {
    const auto button = graphics::components::Component::button("Run", true, false, 10).render();
    const auto radio = graphics::components::Component::radio_button("Option A", true).render();
    const auto checkbox = graphics::components::Component::check_box("Enable Logs", true).render();

    ASSERT_EQ(button.size(), 1U);
    EXPECT_NE(button.front().find("Run"), std::string::npos);
    EXPECT_EQ(radio.front(), "(o) Option A");
    EXPECT_EQ(checkbox.front(), "[x] Enable Logs");
}

TEST(GraphicsComponentsTest, RendersTextAndEditableViews) {
    const auto text_lines = graphics::components::Component::text_view(
        "A text view should wrap descriptive content for panels.", 18).render();
    const auto editable_lines = graphics::components::Component::editable_text_view("Hello", 5, true, 12).render();

    EXPECT_GE(text_lines.size(), 2U);
    EXPECT_NE(editable_lines.front().find("Hello|"), std::string::npos);
}

TEST(GraphicsComponentsTest, RendersVerticalHorizontalAndGridHolders) {
    const auto vertical = graphics::components::ComponentHolder::vertical({
        graphics::components::Component::button("One"),
        graphics::components::Component::check_box("Two", true)
    });
    const auto horizontal = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::radio_button("Left", true),
        graphics::components::Component::radio_button("Right", false)
    }, 2);
    const auto grid = graphics::components::ComponentHolder::grid({
        graphics::components::Component::button("A"),
        graphics::components::Component::button("B"),
        graphics::components::Component::button("C")
    }, 2, 2, 1);

    const auto vertical_lines = vertical.render();
    const auto horizontal_lines = horizontal.render();
    const auto grid_lines = grid.render();

    EXPECT_GE(vertical_lines.size(), 3U);
    EXPECT_NE(vertical_lines.front().find("One"), std::string::npos);
    EXPECT_NE(vertical_lines.back().find("Two"), std::string::npos);

    ASSERT_FALSE(horizontal_lines.empty());
    EXPECT_NE(horizontal_lines.front().find("Left"), std::string::npos);
    EXPECT_NE(horizontal_lines.front().find("Right"), std::string::npos);

    EXPECT_GE(grid_lines.size(), 3U);
    EXPECT_NE(grid_lines.front().find("A"), std::string::npos);
    EXPECT_NE(grid_lines.front().find("B"), std::string::npos);
    EXPECT_NE(grid_lines.back().find("C"), std::string::npos);
}

TEST(GraphicsComponentsTest, RendersLayoutGroupAsComponentType) {
    const auto nested_holder = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::button("Yes"),
        graphics::components::Component::button("No")
    }, 2);

    const auto layout_group = graphics::components::Component::layout_group(nested_holder, "Decision Row");
    const auto rendered = layout_group.render();

    EXPECT_EQ(graphics::components::component_type_name(layout_group.type()), "layoutGroup");
    ASSERT_GE(rendered.size(), 2U);
    EXPECT_NE(rendered.front().find("Decision Row"), std::string::npos);
    EXPECT_NE(rendered.back().find("Yes"), std::string::npos);
    EXPECT_NE(rendered.back().find("No"), std::string::npos);
}

TEST(GraphicsComponentsTest, RendersMenuBarAndDropdownIngredients) {
    graphics::components::MenuModel file_menu("File");
    file_menu
        .add_item(graphics::components::MenuItem::action("New", "Ctrl+N"))
        .add_item(graphics::components::MenuItem::action("Open", "Ctrl+O"))
        .add_item(graphics::components::MenuItem::divider())
        .add_item(graphics::components::MenuItem::action("Export", "Ctrl+E", true, false, true))
        .set_highlighted(true);

    graphics::components::MenuModel edit_menu("Edit");
    edit_menu.add_item(graphics::components::MenuItem::action("Undo", "Ctrl+Z"));

    graphics::components::MenuBarModel menu_bar;
    menu_bar.add_menu(file_menu).add_menu(edit_menu).set_spacing(2);

    const auto bar_lines = graphics::components::Component::menu_bar(menu_bar).render();
    const auto dropdown_lines = graphics::components::Component::dropdown_menu(file_menu, 20).render();

    ASSERT_EQ(bar_lines.size(), 1U);
    EXPECT_NE(bar_lines.front().find("[File]"), std::string::npos);
    EXPECT_NE(bar_lines.front().find("Edit"), std::string::npos);

    ASSERT_GE(dropdown_lines.size(), 6U);
    EXPECT_NE(dropdown_lines[1].find("File"), std::string::npos);
    EXPECT_NE(dropdown_lines[3].find("New"), std::string::npos);
    EXPECT_NE(dropdown_lines[3].find("Ctrl+N"), std::string::npos);
    EXPECT_NE(dropdown_lines[5].find("Export"), std::string::npos);
    EXPECT_NE(dropdown_lines[5].find("▶"), std::string::npos);
}
