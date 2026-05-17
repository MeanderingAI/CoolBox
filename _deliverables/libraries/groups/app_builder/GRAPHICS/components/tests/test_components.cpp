#include "components.hpp"
#include "tyst_framework.hpp"

#include <type_traits>

namespace {

void assert_graphics_object_contract(const graphics::GraphicsObject& object,
                                     const std::string& expected_kind,
                                     const std::string& expected_name) {
    TYST_EXPECT_EQ(object.graphics_object_kind(), expected_kind);
    TYST_EXPECT_EQ(object.graphics_object_name(), expected_name);
}

static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::ToolbarModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::DockPanelModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::LayerListModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::PropertyInspectorModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::FileTreeModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::FileTreeModel::Node>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::RadioSelectorModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::CheckboxGroupModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::MenuItem>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::MenuModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::MenuBarModel>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::Component>);
static_assert(std::is_base_of_v<graphics::GraphicsObject, graphics::components::ComponentHolder>);

} // namespace

TYST_TEST(GraphicsComponentsTest, RendersToolbarDockPanelLayerListPropertyInspectorFileTreeRadioCheckboxGroup) {
    using namespace graphics::components;
    // Toolbar
    ToolbarModel toolbar({"New", "Open", "Save"}, 1);
    auto toolbar_lines = Component::toolbar(toolbar).render();
    TYST_ASSERT_EQ(toolbar_lines.size(), 1U);
    TYST_EXPECT_NE(toolbar_lines.front().find("{New}"), std::string::npos);
    TYST_EXPECT_NE(toolbar_lines.front().find("{Open}"), std::string::npos);
    TYST_EXPECT_NE(toolbar_lines.front().find("{Save}"), std::string::npos);

    // DockPanel
    DockPanelModel dock_panel("Layers", true);
    auto dock_panel_lines = Component::dock_panel(dock_panel).render();
    TYST_ASSERT_EQ(dock_panel_lines.size(), 1U);
    TYST_EXPECT_NE(dock_panel_lines.front().find("[DockPanel] Layers [floating]"), std::string::npos);

    // LayerList
    LayerListModel layers({"Background", "Sketch", "Ink"}, 1);
    auto layer_lines = Component::layer_list(layers).render();
    TYST_ASSERT_GE(layer_lines.size(), 2U);
    TYST_EXPECT_NE(layer_lines[1].find("> Sketch"), std::string::npos);

    // PropertyInspector
    PropertyInspectorModel props;
    props.add_property("Color", "Black").add_property("Width", "2px");
    auto prop_lines = Component::property_inspector(props).render();
    TYST_ASSERT_GE(prop_lines.size(), 2U);
    TYST_EXPECT_NE(prop_lines[1].find("Color: Black"), std::string::npos);

    // FileTree
    FileTreeModel::Node root{"root", true, {
        {"file1.txt", false, {}},
        {"dir1", true, {{"file2.txt", false, {}}}}
    }};
    FileTreeModel file_tree(root);
    auto file_tree_lines = Component::file_tree(file_tree).render();
    TYST_ASSERT_GE(file_tree_lines.size(), 3U);
    TYST_EXPECT_NE(file_tree_lines[0].find("[D] root"), std::string::npos);
    TYST_EXPECT_NE(file_tree_lines[1].find("file1.txt"), std::string::npos);
    TYST_EXPECT_NE(file_tree_lines[2].find("[D] dir1"), std::string::npos);

    // RadioSelector
    RadioSelectorModel radios({"A", "B", "C"}, 2);
    auto radio_lines = Component::radio_selector(radios).render();
    TYST_ASSERT_EQ(radio_lines.size(), 3U);
    TYST_EXPECT_EQ(radio_lines[2], "(o) C");

    // CheckboxGroup
    CheckboxGroupModel checks({"X", "Y", "Z"}, {true, false, true});
    auto check_lines = Component::checkbox_group(checks).render();
    TYST_ASSERT_EQ(check_lines.size(), 3U);
    TYST_EXPECT_EQ(check_lines[0], "[x] X");
    TYST_EXPECT_EQ(check_lines[1], "[ ] Y");
    TYST_EXPECT_EQ(check_lines[2], "[x] Z");
}

TYST_TEST(GraphicsComponentsTest, RendersBasicWidgets) {
    const auto button = graphics::components::Component::button("Run", true, false, 10).render();
    const auto radio = graphics::components::Component::radio_button("Option A", true).render();
    const auto checkbox = graphics::components::Component::check_box("Enable Logs", true).render();

    TYST_ASSERT_EQ(button.size(), 1U);
    TYST_EXPECT_NE(button.front().find("Run"), std::string::npos);
    TYST_EXPECT_EQ(radio.front(), "(o) Option A");
    TYST_EXPECT_EQ(checkbox.front(), "[x] Enable Logs");
}

TYST_TEST(GraphicsComponentsTest, RendersButtonWithIndependentRoundedCorners) {
    const auto rounded = graphics::components::Component::button(
        "Run", true, false, 10, true, false, true, false).render();

    TYST_ASSERT_EQ(rounded.size(), 1U);
    TYST_EXPECT_EQ(rounded.front().front(), '/');
    TYST_EXPECT_EQ(rounded.front().back(), '/');
    TYST_EXPECT_NE(rounded.front().find("Run"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersTextAndEditableViews) {
    const auto text_lines = graphics::components::Component::text_view(
        "A text view should wrap descriptive content for panels.", 18).render();
    const auto editable_lines = graphics::components::Component::editable_text_view("Hello", 5, true, 12).render();
    const auto editable_advanced = graphics::components::Component::editable_text_view(
        "The editable control now supports autocomplete and metadata.",
        10,
        true,
        20,
        true,
        graphics::components::BorderStyle::None,
        0,
        false,
        true,
        3,
        18,
        {"Hello", "Help", "Helium"}).render();

    TYST_EXPECT_GE(text_lines.size(), 2U);
    TYST_EXPECT_NE(editable_lines.front().find("Hello|"), std::string::npos);
    TYST_EXPECT_NE(editable_advanced.back().find("suggestions:"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersBordersAndLineNumbersForTextViews) {
    using namespace graphics::components;

    const auto bordered_text = Component::text_view(
        "first second third fourth fifth",
        10,
        true,
        BorderStyle::Solid,
        1,
        true).render();

    const auto bordered_editable = Component::editable_text_view(
        "alpha beta gamma",
        5,
        true,
        10,
        true,
        BorderStyle::Dashed,
        2,
        true,
        true,
        3,
        14,
        {"alpha", "beta"}).render();

    TYST_ASSERT_GE(bordered_text.size(), 4U);
    TYST_EXPECT_NE(bordered_text.front().find("+"), std::string::npos);
    TYST_EXPECT_NE(bordered_text[1].find("1:"), std::string::npos);

    TYST_ASSERT_GE(bordered_editable.size(), 6U);
    TYST_EXPECT_NE(bordered_editable.front().find("~"), std::string::npos);
    TYST_EXPECT_NE(bordered_editable[2].find("1:"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersTabbedViewWithSubviewContent) {
    using namespace graphics::components;

    TabbedViewModel model;
    model
        .add_tab("Design", ComponentHolder::vertical({Component::button("Draw"), Component::check_box("Snap", true)}))
        .add_tab("Inspect", ComponentHolder::vertical({Component::text_view("Properties"), Component::button("Refresh")}))
        .set_selected(1);

    const auto lines = Component::tabbed_view(model).render();
    TYST_ASSERT_GE(lines.size(), 2U);
    TYST_EXPECT_NE(lines.front().find("[*Inspect]"), std::string::npos);
    TYST_EXPECT_NE(lines[1].find("Properties"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersVerticalHorizontalAndGridHolders) {
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

    TYST_EXPECT_GE(vertical_lines.size(), 3U);
    TYST_EXPECT_NE(vertical_lines.front().find("One"), std::string::npos);
    TYST_EXPECT_NE(vertical_lines.back().find("Two"), std::string::npos);

    TYST_ASSERT_FALSE(horizontal_lines.empty());
    TYST_EXPECT_NE(horizontal_lines.front().find("Left"), std::string::npos);
    TYST_EXPECT_NE(horizontal_lines.front().find("Right"), std::string::npos);

    TYST_EXPECT_GE(grid_lines.size(), 3U);
    TYST_EXPECT_NE(grid_lines.front().find("A"), std::string::npos);
    TYST_EXPECT_NE(grid_lines.front().find("B"), std::string::npos);
    TYST_EXPECT_NE(grid_lines.back().find("C"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersLayoutGroupAsComponentType) {
    const auto nested_holder = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::button("Yes"),
        graphics::components::Component::button("No")
    }, 2);

    const auto layout_group = graphics::components::Component::layout_group(nested_holder, "Decision Row");
    const auto rendered = layout_group.render();

    TYST_EXPECT_EQ(graphics::components::component_type_name(layout_group.type()), "layoutGroup");
    TYST_ASSERT_GE(rendered.size(), 2U);
    TYST_EXPECT_NE(rendered.front().find("Decision Row"), std::string::npos);
    TYST_EXPECT_NE(rendered.back().find("Yes"), std::string::npos);
    TYST_EXPECT_NE(rendered.back().find("No"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, RendersMenuBarAndDropdownIngredients) {
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

    TYST_ASSERT_EQ(bar_lines.size(), 1U);
    TYST_EXPECT_NE(bar_lines.front().find("[File]"), std::string::npos);
    TYST_EXPECT_NE(bar_lines.front().find("Edit"), std::string::npos);

    TYST_ASSERT_GE(dropdown_lines.size(), 6U);
    TYST_EXPECT_NE(dropdown_lines[1].find("File"), std::string::npos);
    TYST_EXPECT_NE(dropdown_lines[3].find("New"), std::string::npos);
    TYST_EXPECT_NE(dropdown_lines[3].find("Ctrl+N"), std::string::npos);
    TYST_EXPECT_NE(dropdown_lines[5].find("Export"), std::string::npos);
    TYST_EXPECT_NE(dropdown_lines[5].find("▶"), std::string::npos);
}

TYST_TEST(GraphicsComponentsTest, SharedBaseContractCoversComponentModels) {
    using namespace graphics::components;

    ToolbarModel toolbar({"New", "Open"}, 1);
    DockPanelModel dock_panel("Explorer", true);
    LayerListModel layers({"Base", "Ink"}, 1);
    PropertyInspectorModel inspector;
    inspector.add_property("Width", "2px");
    FileTreeModel::Node node("workspace", true, {});
    FileTreeModel tree(node);
    RadioSelectorModel radios({"Left", "Right"}, 1);
    CheckboxGroupModel checks({"Track", "Preview"}, {true, false});
    MenuItem item = MenuItem::action("Open", "Ctrl+O");
    MenuModel menu("File");
    menu.add_item(item);
    MenuBarModel menu_bar;
    menu_bar.add_menu(menu);
    Component button = Component::button("Run", true, false, 10);
    ComponentHolder holder = ComponentHolder::vertical({button}, 0);

    assert_graphics_object_contract(toolbar, "toolbarModel", "New");
    assert_graphics_object_contract(dock_panel, "dockPanelModel", "Explorer");
    assert_graphics_object_contract(layers, "layerListModel", "Ink");
    assert_graphics_object_contract(inspector, "propertyInspectorModel", "Width");
    assert_graphics_object_contract(node, "fileTreeNode", "workspace");
    assert_graphics_object_contract(tree, "fileTreeModel", "workspace");
    assert_graphics_object_contract(radios, "radioSelectorModel", "Right");
    assert_graphics_object_contract(checks, "checkboxGroupModel", "Track");
    assert_graphics_object_contract(item, "menuItem", "Open");
    assert_graphics_object_contract(menu, "menuModel", "File");
    assert_graphics_object_contract(menu_bar, "menuBarModel", "File");
    assert_graphics_object_contract(button, "component", "Run");
    assert_graphics_object_contract(holder, "componentHolder", "vertical");
}

TYST_TEST(GraphicsComponentsTest, RegistryCreatesGraphicsObjectsPolymorphically) {
    graphics::GraphicsObjectRegistry registry;

    TYST_ASSERT_EQ(registry.register_type<graphics::components::ToolbarModel>("toolbarModel"), true);
    TYST_ASSERT_EQ(registry.register_type<graphics::components::MenuBarModel>("menuBarModel"), true);
    TYST_ASSERT_EQ(registry.register_factory("primaryButton", []() {
        return std::make_unique<graphics::components::Component>(
            graphics::components::Component::button("Registry Run", true, false, 12));
    }), true);

    TYST_ASSERT_EQ(registry.contains("toolbarModel"), true);
    TYST_ASSERT_GE(registry.registered_keys().size(), 3U);

    const auto toolbar = registry.create("toolbarModel");
    const auto menu_bar = registry.create("menuBarModel");
    const auto button = registry.create("primaryButton");

    assert_graphics_object_contract(*toolbar, "toolbarModel", "toolbar");
    assert_graphics_object_contract(*menu_bar, "menuBarModel", "menuBar");
    assert_graphics_object_contract(*button, "component", "Registry Run");
}
