#ifndef COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP
#define COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
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
    DropdownMenu,
    Toolbar,
    DockPanel,
    LayerList,
    PropertyInspector,
    FileTree,
    RadioSelector,
    CheckboxGroup
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

} // namespace components
} // namespace graphics

#endif  // COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_COMPONENTS_HEADERS_COMPONENTS_HPP