#include "../headers/components.hpp"

#include <algorithm>
#include <sstream>
#include <functional>

namespace graphics {
namespace components {
namespace {

std::string pad_right(const std::string& value, std::size_t width) {
    if (value.size() >= width) {
        return value.substr(0, width);
    }
    return value + std::string(width - value.size(), ' ');
}

std::vector<std::string> wrap_text_lines(const std::string& text, std::size_t width) {
    const std::size_t safe_width = std::max<std::size_t>(width, 8);
    std::istringstream words(text);
    std::vector<std::string> lines;
    std::string word;
    std::string current;

    while (words >> word) {
        if (current.empty()) {
            current = word;
            continue;
        }
        if (current.size() + 1 + word.size() <= safe_width) {
            current += " " + word;
        } else {
            lines.push_back(pad_right(current, safe_width));
            current = word;
        }
    }

    if (!current.empty()) {
        lines.push_back(pad_right(current, safe_width));
    }
    if (lines.empty()) {
        lines.push_back(std::string(safe_width, ' '));
    }
    return lines;
}

std::size_t max_line_width(const std::vector<std::string>& lines) {
    std::size_t width = 0;
    for (const auto& line : lines) {
        width = std::max(width, line.size());
    }
    return width;
}

std::vector<std::string> pad_lines(const std::vector<std::string>& lines, std::size_t width, std::size_t height) {
    std::vector<std::string> padded;
    padded.reserve(height);
    for (std::size_t i = 0; i < lines.size() && padded.size() < height; ++i) {
        padded.push_back(pad_right(lines[i], width));
    }
    while (padded.size() < height) {
        padded.push_back(std::string(width, ' '));
    }
    return padded;

}

std::vector<std::string> join_horizontal(const std::vector<std::vector<std::string>>& blocks,
                                         std::size_t spacing) {
    if (blocks.empty()) {
        return {};
    }

    std::vector<std::size_t> widths;
    widths.reserve(blocks.size());
    std::size_t max_height = 0;
    for (const auto& block : blocks) {
        widths.push_back(max_line_width(block));
        max_height = std::max(max_height, block.size());
    }

    std::vector<std::vector<std::string>> normalized;
    normalized.reserve(blocks.size());
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        normalized.push_back(pad_lines(blocks[i], widths[i], max_height));
    }

    std::vector<std::string> lines;
    lines.reserve(max_height);
    for (std::size_t row = 0; row < max_height; ++row) {
        std::string line;
        for (std::size_t block = 0; block < normalized.size(); ++block) {
            if (block != 0) {
                line += std::string(spacing, ' ');
            }
            line += normalized[block][row];
        }
        lines.push_back(line);
    }
    return lines;
}

std::string render_menu_item_label(const MenuItem& item) {
    if (item.separator) {
        return "";
    }

    std::string label;
    if (item.checked) {
        label += "✓ ";
    } else {
        label += "  ";
    }
    label += item.label;
    if (item.has_submenu) {
        label += " ▶";
    }
    if (!item.enabled) {
        label += " [disabled]";
    }
    return label;
}

std::vector<std::string> render_dropdown_lines(const MenuModel& menu, std::size_t min_width) {
    std::size_t width = std::max<std::size_t>(menu.title.size() + 2, min_width);
    for (const auto& item : menu.items) {
        if (!item.separator) {
            width = std::max(width, render_menu_item_label(item).size() + (item.shortcut.empty() ? 0U : item.shortcut.size() + 2U));
        }
    }

    std::vector<std::string> lines;
    lines.push_back("+" + std::string(width + 2, '-') + "+");
    lines.push_back("| " + pad_right(menu.title, width) + " |");
    lines.push_back("+" + std::string(width + 2, '-') + "+");

    for (const auto& item : menu.items) {
        if (item.separator) {
            continue;
        }

        const std::string shortcut = item.shortcut.empty() ? "" : "  " + item.shortcut;
        const std::size_t content_width = shortcut.size() > width ? 0 : width - shortcut.size();
        lines.push_back("| " + pad_right(render_menu_item_label(item), content_width) + shortcut + " |");
    }

    lines.push_back("+" + std::string(width + 2, '-') + "+");
    return lines;
}

struct BorderGlyphs {
    char corner = '+';
    char horizontal = '-';
    char vertical = '|';
};

BorderGlyphs border_glyphs(BorderStyle style) {
    switch (style) {
        case BorderStyle::Solid: return {'+', '-', '|'};
        case BorderStyle::Dashed: return {'+', '~', ':'};
        case BorderStyle::Double: return {'#', '=', '#'};
        case BorderStyle::None: break;
    }
    return {' ', ' ', ' '};
}

std::vector<std::string> add_line_numbers(const std::vector<std::string>& lines) {
    std::vector<std::string> out;
    out.reserve(lines.size());
    const std::size_t digits = std::to_string(std::max<std::size_t>(1, lines.size())).size();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out.push_back(pad_right(std::to_string(i + 1), digits) + ": " + lines[i]);
    }
    return out;
}

std::vector<std::string> add_border(const std::vector<std::string>& lines,
                                    BorderStyle style,
                                    std::size_t thickness) {
    if (style == BorderStyle::None || thickness == 0 || lines.empty()) {
        return lines;
    }

    const auto glyphs = border_glyphs(style);
    const std::size_t content_width = max_line_width(lines);
    const std::string side(thickness, glyphs.vertical);
    const std::size_t full_width = thickness + 1 + content_width + 1 + thickness;
    const std::string cap(1, glyphs.corner);
    const std::string horizontal = cap + std::string(full_width - 2, glyphs.horizontal) + cap;

    std::vector<std::string> out;
    out.reserve(lines.size() + thickness * 2);
    for (std::size_t i = 0; i < thickness; ++i) {
        out.push_back(horizontal);
    }
    for (const auto& line : lines) {
        out.push_back(side + " " + pad_right(line, content_width) + " " + side);
    }
    for (std::size_t i = 0; i < thickness; ++i) {
        out.push_back(horizontal);
    }
    return out;
}

} // namespace

std::string component_type_name(ComponentType type) {
    switch (type) {
        case ComponentType::Button: return "button";
        case ComponentType::TextView: return "textView";
        case ComponentType::EditableTextView: return "editableTextView";
        case ComponentType::RadioButton: return "radioButton";
        case ComponentType::CheckBox: return "checkBox";
        case ComponentType::LayoutGroup: return "layoutGroup";
        case ComponentType::MenuBar: return "menuBar";
        case ComponentType::DropdownMenu: return "dropdownMenu";
        case ComponentType::Toolbar: return "toolbar";
        case ComponentType::DockPanel: return "dockPanel";
        case ComponentType::LayerList: return "layerList";
        case ComponentType::PropertyInspector: return "propertyInspector";
        case ComponentType::FileTree: return "fileTree";
        case ComponentType::RadioSelector: return "radioSelector";
        case ComponentType::CheckboxGroup: return "checkboxGroup";
        case ComponentType::ColorPicker: return "colorPicker";
        case ComponentType::TabbedView: return "tabbedView";
    }
    return "unknown";
}

// ----- Component factory implementations -----
Component Component::toolbar(const ToolbarModel& toolbar) {
    Component c;
    c.type_ = ComponentType::Toolbar;
    c.toolbar_model_ = std::make_shared<ToolbarModel>(toolbar);
    return c;
}

Component Component::dock_panel(const DockPanelModel& dock_panel) {
    Component c;
    c.type_ = ComponentType::DockPanel;
    c.dock_panel_model_ = std::make_shared<DockPanelModel>(dock_panel);
    return c;
}

Component Component::layer_list(const LayerListModel& layer_list) {
    Component c;
    c.type_ = ComponentType::LayerList;
    c.layer_list_model_ = std::make_shared<LayerListModel>(layer_list);
    return c;
}

Component Component::property_inspector(const PropertyInspectorModel& inspector) {
    Component c;
    c.type_ = ComponentType::PropertyInspector;
    c.property_inspector_model_ = std::make_shared<PropertyInspectorModel>(inspector);
    return c;
}

Component Component::file_tree(const FileTreeModel& file_tree) {
    Component c;
    c.type_ = ComponentType::FileTree;
    c.file_tree_model_ = std::make_shared<FileTreeModel>(file_tree);
    return c;
}

Component Component::radio_selector(const RadioSelectorModel& radio_selector) {
    Component c;
    c.type_ = ComponentType::RadioSelector;
    c.radio_selector_model_ = std::make_shared<RadioSelectorModel>(radio_selector);
    return c;
}

Component Component::checkbox_group(const CheckboxGroupModel& checkbox_group) {
    Component c;
    c.type_ = ComponentType::CheckboxGroup;
    c.checkbox_group_model_ = std::make_shared<CheckboxGroupModel>(checkbox_group);
    return c;
}

Component Component::color_picker(const ColorPickerModel& color_picker) {
    Component c;
    c.type_ = ComponentType::ColorPicker;
    c.color_picker_model_ = std::make_shared<ColorPickerModel>(color_picker);
    return c;
}

Component Component::tabbed_view(const TabbedViewModel& tabbed_view) {
    Component c;
    c.type_ = ComponentType::TabbedView;
    c.tabbed_view_model_ = std::make_shared<TabbedViewModel>(tabbed_view);
    return c;
}
std::string layout_type_name(LayoutType type) {
    switch (type) {
        case LayoutType::Vertical: return "vertical";
        case LayoutType::Horizontal: return "horizontal";
        case LayoutType::Grid: return "grid";
    }
    return "vertical";
}

std::string border_style_name(BorderStyle style) {
    switch (style) {
        case BorderStyle::None: return "none";
        case BorderStyle::Solid: return "solid";
        case BorderStyle::Dashed: return "dashed";
        case BorderStyle::Double: return "double";
    }
    return "none";
}

MenuItem MenuItem::action(std::string label, std::string shortcut, bool enabled, bool checked, bool has_submenu) {
    MenuItem item;
    item.label = std::move(label);
    item.shortcut = std::move(shortcut);
    item.enabled = enabled;
    item.checked = checked;
    item.has_submenu = has_submenu;
    return item;
}

MenuItem MenuItem::divider() {
    MenuItem item;
    item.separator = true;
    item.enabled = false;
    return item;
}

MenuModel::MenuModel(std::string menu_title) : title(std::move(menu_title)) {}

MenuModel& MenuModel::add_item(const MenuItem& item) {
    items.push_back(item);
    return *this;
}

MenuModel& MenuModel::set_highlighted(bool highlighted_state) {
    highlighted = highlighted_state;
    return *this;
}

MenuBarModel::MenuBarModel(std::vector<MenuModel> menu_models, std::size_t spacing_value)
    : menus(std::move(menu_models)), spacing(spacing_value) {}

MenuBarModel& MenuBarModel::add_menu(const MenuModel& menu) {
    menus.push_back(menu);
    return *this;
}

MenuBarModel& MenuBarModel::set_spacing(std::size_t spacing_value) {
    spacing = spacing_value;
    return *this;
}

Component Component::button(std::string label,
                            bool enabled,
                            bool pressed,
                            std::size_t width,
                            bool round_top_left,
                            bool round_top_right,
                            bool round_bottom_right,
                            bool round_bottom_left) {
    Component component;
    component.type_ = ComponentType::Button;
    component.label_ = std::move(label);
    component.enabled_ = enabled;
    component.pressed_ = pressed;
    component.width_ = width;
    component.round_top_left_ = round_top_left;
    component.round_top_right_ = round_top_right;
    component.round_bottom_right_ = round_bottom_right;
    component.round_bottom_left_ = round_bottom_left;
    return component;
}

Component Component::text_view(std::string text,
                               std::size_t width,
                               bool wrap_text,
                               BorderStyle border_style,
                               std::size_t border_thickness,
                               bool show_line_numbers) {
    Component component;
    component.type_ = ComponentType::TextView;
    component.text_ = std::move(text);
    component.width_ = width;
    component.wrap_text_ = wrap_text;
    component.border_style_ = border_style;
    component.border_thickness_ = border_thickness;
    component.show_line_numbers_ = show_line_numbers;
    return component;
}

Component Component::editable_text_view(std::string text,
                                        std::size_t cursor_position,
                                        bool focused,
                                        std::size_t width,
                                        bool wrap_text,
                                        BorderStyle border_style,
                                        std::size_t border_thickness,
                                        bool show_line_numbers,
                                        bool resizable,
                                        std::size_t max_height,
                                        std::size_t font_size,
                                        std::vector<std::string> autocomplete_suggestions) {
    Component component;
    component.type_ = ComponentType::EditableTextView;
    component.text_ = std::move(text);
    component.cursor_position_ = cursor_position;
    component.focused_ = focused;
    component.width_ = width;
    component.wrap_text_ = wrap_text;
    component.border_style_ = border_style;
    component.border_thickness_ = border_thickness;
    component.show_line_numbers_ = show_line_numbers;
    component.resizable_ = resizable;
    component.max_height_ = std::max<std::size_t>(1, max_height);
    component.font_size_ = std::max<std::size_t>(8, font_size);
    component.autocomplete_suggestions_ = std::move(autocomplete_suggestions);
    return component;
}

Component Component::radio_button(std::string label, bool selected, bool enabled) {
    Component component;
    component.type_ = ComponentType::RadioButton;
    component.label_ = std::move(label);
    component.selected_ = selected;
    component.enabled_ = enabled;
    return component;
}

Component Component::check_box(std::string label, bool checked, bool enabled) {
    Component component;
    component.type_ = ComponentType::CheckBox;
    component.label_ = std::move(label);
    component.checked_ = checked;
    component.enabled_ = enabled;
    return component;
}

Component Component::menu_bar(const MenuBarModel& menu_bar) {
    Component component;
    component.type_ = ComponentType::MenuBar;
    component.menu_bar_model_ = std::make_shared<MenuBarModel>(menu_bar);
    return component;
}

Component Component::dropdown_menu(const MenuModel& menu, std::size_t min_width) {
    Component component;
    component.type_ = ComponentType::DropdownMenu;
    component.dropdown_menu_model_ = std::make_shared<MenuModel>(menu);
    component.width_ = min_width;
    return component;
}

Component Component::layout_group(const ComponentHolder& holder, std::string label) {
    Component component;
    component.type_ = ComponentType::LayoutGroup;
    component.label_ = std::move(label);
    component.layout_group_ = std::make_shared<ComponentHolder>(holder);
    return component;
}

const ToolbarModel* Component::toolbar_model() const { return toolbar_model_.get(); }
const DockPanelModel* Component::dock_panel_model() const { return dock_panel_model_.get(); }
const LayerListModel* Component::layer_list_model() const { return layer_list_model_.get(); }
const PropertyInspectorModel* Component::property_inspector_model() const { return property_inspector_model_.get(); }
const FileTreeModel* Component::file_tree_model() const { return file_tree_model_.get(); }
const RadioSelectorModel* Component::radio_selector_model() const { return radio_selector_model_.get(); }
const CheckboxGroupModel* Component::checkbox_group_model() const { return checkbox_group_model_.get(); }
const ColorPickerModel* Component::color_picker_model() const { return color_picker_model_.get(); }
const TabbedViewModel* Component::tabbed_view_model() const { return tabbed_view_model_.get(); }

std::vector<std::string> Component::render() const {
    switch (type_) {
        case ComponentType::Button: {
            const std::size_t inner_width = std::max<std::size_t>(width_ > 2 ? width_ - 2 : 0, label_.size() + 2);
            const std::string padded = pad_right(" " + label_ + " ", inner_width);
            const bool rounded = round_top_left_ || round_top_right_ || round_bottom_right_ || round_bottom_left_;
            if (rounded) {
                const char left = round_top_left_ && round_bottom_left_
                    ? '('
                    : (round_top_left_ ? '/' : (round_bottom_left_ ? '\\' : '['));
                const char right = round_top_right_ && round_bottom_right_
                    ? ')'
                    : (round_top_right_ ? '\\' : (round_bottom_right_ ? '/' : ']'));
                std::string line;
                line.push_back(left);
                line += padded;
                line.push_back(right);
                if (!enabled_) {
                    line += " [disabled]";
                } else if (pressed_) {
                    line += " [pressed]";
                }
                return {line};
            }
            if (!enabled_) {
                return {"(" + padded + ")"};
            }
            if (pressed_) {
                return {"<" + padded + ">"};
            }
            return {"[" + padded + "]"};
        }
        case ComponentType::TextView: {
            std::vector<std::string> lines;
            if (wrap_text_) {
                lines = wrap_text_lines(text_, width_ == 0 ? 32 : width_);
            } else {
                lines = {pad_right(text_, width_ == 0 ? 32 : width_)};
            }
            if (show_line_numbers_) {
                lines = add_line_numbers(lines);
            }
            return add_border(lines, border_style_, border_thickness_);
        }
        case ComponentType::EditableTextView: {
            const std::size_t safe_width = std::max<std::size_t>(width_ == 0 ? 24 : width_, 8);
            const std::size_t effective_width = std::max<std::size_t>(8, (safe_width * 14U) / std::max<std::size_t>(font_size_, 8));
            std::string editable = text_;
            const std::size_t cursor = std::min(cursor_position_, editable.size());
            if (focused_) {
                editable.insert(cursor, "|");
            }
            if (wrap_text_) {
                const auto lines = wrap_text_lines(editable, effective_width);
                std::vector<std::string> wrapped;
                const std::size_t line_limit = std::min<std::size_t>(lines.size(), max_height_);
                wrapped.reserve(line_limit + 2U);
                for (std::size_t i = 0; i < line_limit; ++i) {
                    const auto& line = lines[i];
                    wrapped.push_back("{" + line + "}");
                }
                if (resizable_) {
                    wrapped.push_back("[resizable max-height=" + std::to_string(max_height_) + " font-size=" + std::to_string(font_size_) + "]");
                }
                if (!autocomplete_suggestions_.empty()) {
                    std::string suggestions = "suggestions: ";
                    for (std::size_t i = 0; i < autocomplete_suggestions_.size(); ++i) {
                        if (i != 0) {
                            suggestions += ", ";
                        }
                        suggestions += autocomplete_suggestions_[i];
                        if (i >= 4) {
                            suggestions += " ...";
                            break;
                        }
                    }
                    wrapped.push_back(suggestions);
                }
                if (show_line_numbers_) {
                    wrapped = add_line_numbers(wrapped);
                }
                return add_border(wrapped, border_style_, border_thickness_);
            }
            editable = pad_right(editable, effective_width);
            std::vector<std::string> result = {"{" + editable + "}"};
            if (resizable_) {
                result.push_back("[resizable max-height=" + std::to_string(max_height_) + " font-size=" + std::to_string(font_size_) + "]");
            }
            if (!autocomplete_suggestions_.empty()) {
                std::string suggestions = "suggestions: ";
                for (std::size_t i = 0; i < autocomplete_suggestions_.size(); ++i) {
                    if (i != 0) {
                        suggestions += ", ";
                    }
                    suggestions += autocomplete_suggestions_[i];
                    if (i >= 4) {
                        suggestions += " ...";
                        break;
                    }
                }
                result.push_back(suggestions);
            }
            if (show_line_numbers_) {
                result = add_line_numbers(result);
            }
            return add_border(result, border_style_, border_thickness_);
        }
        case ComponentType::RadioButton:
            return {(selected_ ? "(o) " : "( ) ") + label_ + (enabled_ ? "" : " [disabled]")};
        case ComponentType::CheckBox:
            return {(checked_ ? "[x] " : "[ ] ") + label_ + (enabled_ ? "" : " [disabled]")};
        case ComponentType::MenuBar: {
            if (!menu_bar_model_ || menu_bar_model_->menus.empty()) {
                return {"<no menus>"};
            }

            std::vector<std::string> labels;
            labels.reserve(menu_bar_model_->menus.size());
            for (const auto& menu : menu_bar_model_->menus) {
                labels.push_back(menu.highlighted ? "[" + menu.title + "]" : menu.title);
            }

            std::string line;
            for (std::size_t i = 0; i < labels.size(); ++i) {
                if (i != 0) {
                    line += std::string(menu_bar_model_->spacing, ' ');
                }
                line += labels[i];
            }
            return {line};
        }
        case ComponentType::DropdownMenu:
            if (!dropdown_menu_model_) {
                return {};
            }
            return render_dropdown_lines(*dropdown_menu_model_, width_);
        case ComponentType::LayoutGroup: {
            if (!layout_group_) {
                return {};
            }
            auto lines = layout_group_->render();
            if (!label_.empty()) {
                lines.insert(lines.begin(), "<LayoutGroup: " + label_ + ">");
            }
            return lines;
        }
        case ComponentType::Toolbar: {
            if (!toolbar_model_) return {"<no toolbar>"};
            std::string line;
            for (std::size_t i = 0; i < toolbar_model_->actions.size(); ++i) {
                if (i != 0) line += std::string(toolbar_model_->spacing, ' ');
                line += "{" + toolbar_model_->actions[i] + "}";
            }
            return {line};
        }
        case ComponentType::DockPanel: {
            if (!dock_panel_model_) return {"<no dock panel>"};
            std::string title = dock_panel_model_->title + (dock_panel_model_->floating ? " [floating]" : "");
            return {"[DockPanel] " + title};
        }
        case ComponentType::LayerList: {
            if (!layer_list_model_) return {"<no layers>"};
            std::vector<std::string> lines;
            for (std::size_t i = 0; i < layer_list_model_->layers.size(); ++i) {
                std::string prefix = (i == layer_list_model_->selected ? "> " : "  ");
                lines.push_back(prefix + layer_list_model_->layers[i]);
            }
            return lines;
        }
        case ComponentType::PropertyInspector: {
            if (!property_inspector_model_) return {"<no properties>"};
            std::vector<std::string> lines = {"Properties:"};
            for (const auto& kv : property_inspector_model_->properties) {
                lines.push_back("- " + kv.first + ": " + kv.second);
            }
            return lines;
        }
        case ComponentType::FileTree: {
            if (!file_tree_model_) return {"<no file tree>"};
            std::vector<std::string> lines;
            std::function<void(const FileTreeModel::Node&, std::string)> walk = [&](const FileTreeModel::Node& n, std::string prefix) {
                lines.push_back(prefix + (n.is_dir ? "[D] " : "    ") + n.name);
                for (const auto& c : n.children) walk(c, prefix + "  ");
            };
            walk(file_tree_model_->root, "");
            return lines;
        }
        case ComponentType::RadioSelector: {
            if (!radio_selector_model_) return {"<no radio selector>"};
            std::vector<std::string> lines;
            for (std::size_t i = 0; i < radio_selector_model_->options.size(); ++i) {
                std::string prefix = (i == radio_selector_model_->selected ? "(o) " : "( ) ");
                lines.push_back(prefix + radio_selector_model_->options[i]);
            }
            return lines;
        }
        case ComponentType::CheckboxGroup: {
            if (!checkbox_group_model_) return {"<no checkbox group>"};
            std::vector<std::string> lines;
            for (std::size_t i = 0; i < checkbox_group_model_->options.size(); ++i) {
                std::string prefix = (i < checkbox_group_model_->checked.size() && checkbox_group_model_->checked[i]) ? "[x] " : "[ ] ";
                lines.push_back(prefix + checkbox_group_model_->options[i]);
            }
            return lines;
        }
        case ComponentType::ColorPicker: {
            if (!color_picker_model_) return {"<no color picker>"};
            std::ostringstream out;
            out << "ColorPicker rgb(" << static_cast<int>(color_picker_model_->r)
                << ", " << static_cast<int>(color_picker_model_->g)
                << ", " << static_cast<int>(color_picker_model_->b) << ")";
            return {out.str()};
        }
        case ComponentType::TabbedView: {
            if (!tabbed_view_model_ || tabbed_view_model_->tabs.empty()) {
                return {"<no tabs>"};
            }
            const std::size_t selected_index = std::min<std::size_t>(tabbed_view_model_->selected, tabbed_view_model_->tabs.size() - 1U);
            std::string tabs_line = "Tabs: ";
            for (std::size_t i = 0; i < tabbed_view_model_->tabs.size(); ++i) {
                if (i != 0) {
                    tabs_line += " | ";
                }
                tabs_line += (i == selected_index ? "[*" : "[");
                tabs_line += tabbed_view_model_->tabs[i].title;
                tabs_line += "]";
            }
            std::vector<std::string> lines;
            lines.push_back(tabs_line);
            const auto selected_view_lines = tabbed_view_model_->tabs[selected_index].subview.render();
            for (const auto& line : selected_view_lines) {
                lines.push_back("  " + line);
            }
            return lines;
        }
    }
    return {};
}

ComponentHolder ComponentHolder::vertical(std::vector<Component> components, std::size_t spacing) {
    ComponentHolder holder;
    holder.layout_ = LayoutType::Vertical;
    holder.components_ = std::move(components);
    holder.vertical_spacing_ = spacing;
    holder.columns_ = 1;
    holder.horizontal_spacing_ = 0;
    return holder;
}

ComponentHolder ComponentHolder::horizontal(std::vector<Component> components, std::size_t spacing) {
    ComponentHolder holder;
    holder.layout_ = LayoutType::Horizontal;
    holder.components_ = std::move(components);
    holder.horizontal_spacing_ = spacing;
    holder.columns_ = holder.components_.empty() ? 1 : holder.components_.size();
    holder.vertical_spacing_ = 0;
    return holder;
}

ComponentHolder ComponentHolder::grid(std::vector<Component> components,
                                      std::size_t columns,
                                      std::size_t horizontal_spacing,
                                      std::size_t vertical_spacing) {
    ComponentHolder holder;
    holder.layout_ = LayoutType::Grid;
    holder.components_ = std::move(components);
    holder.columns_ = std::max<std::size_t>(columns, 1);
    holder.horizontal_spacing_ = horizontal_spacing;
    holder.vertical_spacing_ = vertical_spacing;
    return holder;
}

ComponentHolder& ComponentHolder::add_component(const Component& component) {
    components_.push_back(component);
    if (layout_ == LayoutType::Horizontal) {
        columns_ = components_.size();
    }
    return *this;
}

std::vector<std::string> ComponentHolder::render() const {
    if (components_.empty()) {
        return {};
    }

    if (layout_ == LayoutType::Vertical) {
        std::vector<std::string> lines;
        for (std::size_t i = 0; i < components_.size(); ++i) {
            const auto rendered = components_[i].render();
            lines.insert(lines.end(), rendered.begin(), rendered.end());
            if (i + 1 < components_.size()) {
                for (std::size_t gap = 0; gap < vertical_spacing_; ++gap) {
                    lines.push_back("");
                }
            }
        }
        return lines;
    }

    if (layout_ == LayoutType::Horizontal) {
        std::vector<std::vector<std::string>> blocks;
        blocks.reserve(components_.size());
        for (const auto& component : components_) {
            blocks.push_back(component.render());
        }
        return join_horizontal(blocks, horizontal_spacing_);
    }

    std::vector<std::string> lines;
    const std::size_t grid_columns = std::max<std::size_t>(columns_, 1);
    for (std::size_t index = 0; index < components_.size(); index += grid_columns) {
        std::vector<std::vector<std::string>> row_blocks;
        for (std::size_t offset = 0; offset < grid_columns && index + offset < components_.size(); ++offset) {
            row_blocks.push_back(components_[index + offset].render());
        }
        const auto row_lines = join_horizontal(row_blocks, horizontal_spacing_);
        lines.insert(lines.end(), row_lines.begin(), row_lines.end());
        if (index + grid_columns < components_.size()) {
            for (std::size_t gap = 0; gap < vertical_spacing_; ++gap) {
                lines.push_back("");
            }
        }
    }
    return lines;
}

} // namespace components
} // namespace graphics
