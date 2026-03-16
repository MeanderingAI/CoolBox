#include "../headers/components.hpp"

#include <algorithm>
#include <sstream>

namespace graphics {
namespace components {
namespace {

std::string pad_right(const std::string& value, std::size_t width) {
    if (value.size() >= width) {
        return value.substr(0, width);
    }
    return value + std::string(width - value.size(), ' ');
}

std::vector<std::string> wrap_text(const std::string& text, std::size_t width) {
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

} // namespace

std::string component_type_name(ComponentType type) {
    switch (type) {
        case ComponentType::Button: return "button";
        case ComponentType::TextView: return "textView";
        case ComponentType::EditableTextView: return "editableTextView";
        case ComponentType::RadioButton: return "radioButton";
        case ComponentType::CheckBox: return "checkBox";
        case ComponentType::LayoutGroup: return "layoutGroup";
    }
    return "unknown";
}

std::string layout_type_name(LayoutType type) {
    switch (type) {
        case LayoutType::Vertical: return "vertical";
        case LayoutType::Horizontal: return "horizontal";
        case LayoutType::Grid: return "grid";
    }
    return "vertical";
}

Component Component::button(std::string label, bool enabled, bool pressed, std::size_t width) {
    Component component;
    component.type_ = ComponentType::Button;
    component.label_ = std::move(label);
    component.enabled_ = enabled;
    component.pressed_ = pressed;
    component.width_ = width;
    return component;
}

Component Component::text_view(std::string text, std::size_t width) {
    Component component;
    component.type_ = ComponentType::TextView;
    component.text_ = std::move(text);
    component.width_ = width;
    return component;
}

Component Component::editable_text_view(std::string text, std::size_t cursor_position, bool focused, std::size_t width) {
    Component component;
    component.type_ = ComponentType::EditableTextView;
    component.text_ = std::move(text);
    component.cursor_position_ = cursor_position;
    component.focused_ = focused;
    component.width_ = width;
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

Component Component::layout_group(const ComponentHolder& holder, std::string label) {
    Component component;
    component.type_ = ComponentType::LayoutGroup;
    component.label_ = std::move(label);
    component.layout_group_ = std::make_shared<ComponentHolder>(holder);
    return component;
}

std::vector<std::string> Component::render() const {
    switch (type_) {
        case ComponentType::Button: {
            const std::size_t inner_width = std::max<std::size_t>(width_ > 2 ? width_ - 2 : 0, label_.size() + 2);
            const std::string padded = pad_right(" " + label_ + " ", inner_width);
            if (!enabled_) {
                return {"(" + padded + ")"};
            }
            if (pressed_) {
                return {"<" + padded + ">"};
            }
            return {"[" + padded + "]"};
        }
        case ComponentType::TextView:
            return wrap_text(text_, width_ == 0 ? 32 : width_);
        case ComponentType::EditableTextView: {
            const std::size_t safe_width = std::max<std::size_t>(width_ == 0 ? 24 : width_, 8);
            std::string editable = text_;
            const std::size_t cursor = std::min(cursor_position_, editable.size());
            if (focused_) {
                editable.insert(cursor, "|");
            }
            editable = pad_right(editable, safe_width);
            return {"{" + editable + "}"};
        }
        case ComponentType::RadioButton:
            return {(selected_ ? "(o) " : "( ) ") + label_ + (enabled_ ? "" : " [disabled]")};
        case ComponentType::CheckBox:
            return {(checked_ ? "[x] " : "[ ] ") + label_ + (enabled_ ? "" : " [disabled]")};
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
