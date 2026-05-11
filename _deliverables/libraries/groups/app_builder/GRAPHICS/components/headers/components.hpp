#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../../graphics_object.hpp"

namespace graphics {
namespace components {

enum class ComponentType {
    Button,
    TextView,
    EditableTextView,
    RadioButton,
    CheckBox,
    LayoutGroup,
    MenuBar,
    DropdownMenu
    ,Toolbar
    ,DockPanel
    ,LayerList
    ,PropertyInspector
    ,FileTree
    ,RadioSelector
    ,CheckboxGroup
};
struct ToolbarModel : public ::graphics::GraphicsObject {
    std::vector<std::string> actions;
    std::size_t spacing = 2;
    ToolbarModel() = default;
    explicit ToolbarModel(std::vector<std::string> actions, std::size_t spacing = 2)
        : actions(std::move(actions)), spacing(spacing) {}
    ToolbarModel& add_action(const std::string& action) { actions.push_back(action); return *this; }
    ToolbarModel& set_spacing(std::size_t s) { spacing = s; return *this; }
    std::string graphics_object_kind() const override { return "toolbarModel"; }
    std::string graphics_object_name() const override {
        return actions.empty() ? std::string("toolbar") : actions.front();
    }
};

struct DockPanelModel : public ::graphics::GraphicsObject {
    std::string title;
    bool floating = false;
    DockPanelModel() = default;
    explicit DockPanelModel(std::string title, bool floating = false)
        : title(std::move(title)), floating(floating) {}
    DockPanelModel& set_floating(bool f) { floating = f; return *this; }
    std::string graphics_object_kind() const override { return "dockPanelModel"; }
    std::string graphics_object_name() const override { return title; }
};

struct LayerListModel : public ::graphics::GraphicsObject {
    std::vector<std::string> layers;
    std::size_t selected = 0;
    LayerListModel() = default;
    explicit LayerListModel(std::vector<std::string> layers, std::size_t selected = 0)
        : layers(std::move(layers)), selected(selected) {}
    LayerListModel& add_layer(const std::string& l) { layers.push_back(l); return *this; }
    LayerListModel& set_selected(std::size_t s) { selected = s; return *this; }
    std::string graphics_object_kind() const override { return "layerListModel"; }
    std::string graphics_object_name() const override {
        return layers.empty() ? std::string("layers") : layers[std::min(selected, layers.size() - 1U)];
    }
};

struct PropertyInspectorModel : public ::graphics::GraphicsObject {
    std::vector<std::pair<std::string, std::string>> properties;
    PropertyInspectorModel() = default;
    explicit PropertyInspectorModel(std::vector<std::pair<std::string, std::string>> props)
        : properties(std::move(props)) {}
    PropertyInspectorModel& add_property(const std::string& k, const std::string& v) { properties.emplace_back(k, v); return *this; }
    std::string graphics_object_kind() const override { return "propertyInspectorModel"; }
    std::string graphics_object_name() const override {
        return properties.empty() ? std::string("properties") : properties.front().first;
    }
};

struct FileTreeModel : public ::graphics::GraphicsObject {
    struct Node : public ::graphics::GraphicsObject {
        std::string name;
        bool is_dir = false;
        std::vector<Node> children;

        Node() = default;
        Node(std::string node_name, bool directory = false, std::vector<Node> node_children = {})
            : name(std::move(node_name)), is_dir(directory), children(std::move(node_children)) {}

        std::string graphics_object_kind() const override { return "fileTreeNode"; }
        std::string graphics_object_name() const override { return name; }
    };
    Node root;
    FileTreeModel() = default;
    explicit FileTreeModel(Node root) : root(std::move(root)) {}
    std::string graphics_object_kind() const override { return "fileTreeModel"; }
    std::string graphics_object_name() const override { return root.name; }
};

struct RadioSelectorModel : public ::graphics::GraphicsObject {
    std::vector<std::string> options;
    std::size_t selected = 0;
    RadioSelectorModel() = default;
    explicit RadioSelectorModel(std::vector<std::string> opts, std::size_t sel = 0)
        : options(std::move(opts)), selected(sel) {}
    RadioSelectorModel& add_option(const std::string& o) { options.push_back(o); return *this; }
    RadioSelectorModel& set_selected(std::size_t s) { selected = s; return *this; }
    std::string graphics_object_kind() const override { return "radioSelectorModel"; }
    std::string graphics_object_name() const override {
        return options.empty() ? std::string("radioSelector") : options[std::min(selected, options.size() - 1U)];
    }
};

struct CheckboxGroupModel : public ::graphics::GraphicsObject {
    std::vector<std::string> options;
    std::vector<bool> checked;
    CheckboxGroupModel() = default;
    explicit CheckboxGroupModel(std::vector<std::string> opts, std::vector<bool> chk = {})
        : options(std::move(opts)), checked(std::move(chk)) {
        if (checked.size() < options.size()) checked.resize(options.size(), false);
    }
    CheckboxGroupModel& add_option(const std::string& o, bool c = false) { options.push_back(o); checked.push_back(c); return *this; }
    CheckboxGroupModel& set_checked(std::size_t i, bool c) { if (i < checked.size()) checked[i] = c; return *this; }
    std::string graphics_object_kind() const override { return "checkboxGroupModel"; }
    std::string graphics_object_name() const override {
        return options.empty() ? std::string("checkboxGroup") : options.front();
    }
};
// Factory functions and model getters are declared on `Component` below.

enum class LayoutType {
    Vertical,
    Horizontal,
    Grid
};

std::string component_type_name(ComponentType type);
std::string layout_type_name(LayoutType type);

struct MenuItem : public ::graphics::GraphicsObject {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    bool checked = false;
    bool has_submenu = false;

    static MenuItem action(std::string label,
                           std::string shortcut = "",
                           bool enabled = true,
                           bool checked = false,
                           bool has_submenu = false);
    static MenuItem divider();

    std::string graphics_object_kind() const override { return "menuItem"; }
    std::string graphics_object_name() const override { return separator ? std::string("divider") : label; }
};

struct MenuModel : public ::graphics::GraphicsObject {
    std::string title;
    std::vector<MenuItem> items;
    bool highlighted = false;

    MenuModel() = default;
    explicit MenuModel(std::string title);

    MenuModel& add_item(const MenuItem& item);
    MenuModel& set_highlighted(bool highlighted_state);

    std::string graphics_object_kind() const override { return "menuModel"; }
    std::string graphics_object_name() const override { return title; }
};

struct MenuBarModel : public ::graphics::GraphicsObject {
    std::vector<MenuModel> menus;
    std::size_t spacing = 3;

    MenuBarModel() = default;
    explicit MenuBarModel(std::vector<MenuModel> menus,
                          std::size_t spacing = 3);

    MenuBarModel& add_menu(const MenuModel& menu);
    MenuBarModel& set_spacing(std::size_t spacing_value);

    std::string graphics_object_kind() const override { return "menuBarModel"; }
    std::string graphics_object_name() const override {
        return menus.empty() ? std::string("menuBar") : menus.front().title;
    }
};

class ComponentHolder;

class Component : public ::graphics::GraphicsObject {
public:
    static Component button(std::string label,
                            bool enabled = true,
                            bool pressed = false,
                            std::size_t width = 0);
    static Component text_view(std::string text,
                               std::size_t width = 32);
    static Component editable_text_view(std::string text,
                                        std::size_t cursor_position = 0,
                                        bool focused = true,
                                        std::size_t width = 32);
    static Component radio_button(std::string label,
                                  bool selected = false,
                                  bool enabled = true);
    static Component check_box(std::string label,
                               bool checked = false,
                               bool enabled = true);
    static Component menu_bar(const MenuBarModel& menu_bar);
    static Component dropdown_menu(const MenuModel& menu,
                                   std::size_t min_width = 0);
    static Component layout_group(const ComponentHolder& holder,
                                  std::string label = "");
    // Additional component factories for complex widgets
    static Component toolbar(const ToolbarModel& toolbar);
    static Component dock_panel(const DockPanelModel& dock_panel);
    static Component layer_list(const LayerListModel& layer_list);
    static Component property_inspector(const PropertyInspectorModel& inspector);
    static Component file_tree(const FileTreeModel& file_tree);
    static Component radio_selector(const RadioSelectorModel& radio_selector);
    static Component checkbox_group(const CheckboxGroupModel& checkbox_group);

    // Accessors for associated models (defined in source file)
    const ToolbarModel* toolbar_model() const;
    const DockPanelModel* dock_panel_model() const;
    const LayerListModel* layer_list_model() const;
    const PropertyInspectorModel* property_inspector_model() const;
    const FileTreeModel* file_tree_model() const;
    const RadioSelectorModel* radio_selector_model() const;
    const CheckboxGroupModel* checkbox_group_model() const;

    ComponentType type() const { return type_; }
    const std::string& label() const { return label_; }
    const std::string& text() const { return text_; }
    bool enabled() const { return enabled_; }
    bool selected() const { return selected_; }
    bool checked() const { return checked_; }
    bool pressed() const { return pressed_; }
    bool focused() const { return focused_; }
    std::size_t width() const { return width_; }
    std::size_t cursor_position() const { return cursor_position_; }
    const ComponentHolder* layout_group() const { return layout_group_.get(); }
    const MenuBarModel* menu_bar_model() const { return menu_bar_model_.get(); }
    const MenuModel* dropdown_menu_model() const { return dropdown_menu_model_.get(); }

    std::string graphics_object_kind() const override { return "component"; }
    std::string graphics_object_name() const override {
        if (!label_.empty()) {
            return label_;
        }
        if (!text_.empty()) {
            return text_;
        }
        return component_type_name(type_);
    }

    std::vector<std::string> render() const;

private:
    ComponentType type_ = ComponentType::TextView;
    std::string label_;
    std::string text_;
    bool enabled_ = true;
    bool selected_ = false;
    bool checked_ = false;
    bool pressed_ = false;
    bool focused_ = false;
    std::size_t width_ = 0;
    std::size_t cursor_position_ = 0;
    std::shared_ptr<ComponentHolder> layout_group_;
    std::shared_ptr<MenuBarModel> menu_bar_model_;
    std::shared_ptr<MenuModel> dropdown_menu_model_;
    std::shared_ptr<ToolbarModel> toolbar_model_;
    std::shared_ptr<DockPanelModel> dock_panel_model_;
    std::shared_ptr<LayerListModel> layer_list_model_;
    std::shared_ptr<PropertyInspectorModel> property_inspector_model_;
    std::shared_ptr<FileTreeModel> file_tree_model_;
    std::shared_ptr<RadioSelectorModel> radio_selector_model_;
    std::shared_ptr<CheckboxGroupModel> checkbox_group_model_;
};

class ComponentHolder : public ::graphics::GraphicsObject {
public:
    static ComponentHolder vertical(std::vector<Component> components = {},
                                    std::size_t spacing = 1);
    static ComponentHolder horizontal(std::vector<Component> components = {},
                                      std::size_t spacing = 3);
    static ComponentHolder grid(std::vector<Component> components = {},
                                std::size_t columns = 2,
                                std::size_t horizontal_spacing = 3,
                                std::size_t vertical_spacing = 1);

    ComponentHolder& add_component(const Component& component);

    LayoutType layout() const { return layout_; }
    const std::vector<Component>& components() const { return components_; }
    std::size_t columns() const { return columns_; }
    std::size_t horizontal_spacing() const { return horizontal_spacing_; }
    std::size_t vertical_spacing() const { return vertical_spacing_; }

    std::string graphics_object_kind() const override { return "componentHolder"; }
    std::string graphics_object_name() const override { return layout_type_name(layout_); }

    std::vector<std::string> render() const;

private:
    LayoutType layout_ = LayoutType::Vertical;
    std::vector<Component> components_;
    std::size_t columns_ = 1;
    std::size_t horizontal_spacing_ = 1;
    std::size_t vertical_spacing_ = 1;
};

} // namespace components
} // namespace graphics
#endif  // COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP
