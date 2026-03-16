#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace graphics {
namespace components {

enum class ComponentType {
    Button,
    TextView,
    EditableTextView,
    RadioButton,
    CheckBox,
    LayoutGroup
};

enum class LayoutType {
    Vertical,
    Horizontal,
    Grid
};

std::string component_type_name(ComponentType type);
std::string layout_type_name(LayoutType type);

class ComponentHolder;

class Component {
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
    static Component layout_group(const ComponentHolder& holder,
                                  std::string label = "");

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
};

class ComponentHolder {
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
