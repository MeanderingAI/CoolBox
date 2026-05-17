#include "../headers/windows.hpp"

#include "json.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace graphics {
namespace windows {
namespace {

std::string repeat(char ch, std::size_t count) {
    return std::string(count, ch);
}

std::string truncate_or_pad(const std::string& text, std::size_t width) {
    if (text.size() >= width) {
        return text.substr(0, width);
    }
    return text + std::string(width - text.size(), ' ');
}

std::string join_menu_titles(const std::vector<Menu>& menus) {
    std::ostringstream stream;
    for (std::size_t i = 0; i < menus.size(); ++i) {
        if (i != 0) {
            stream << " | ";
        }
        stream << menus[i].title;
    }
    return stream.str();
}

std::string platform_label(PlatformStyle platform) {
    switch (platform) {
        case PlatformStyle::MacOS: return "macOS";
        case PlatformStyle::Linux: return "Linux";
        case PlatformStyle::Windows: return "Windows";
    }
    return "Unknown";
}

dataformats::json::Array to_json_array(const std::vector<std::string>& values) {
    dataformats::json::Array array;
    for (const auto& value : values) {
        array.push(dataformats::json::Value(value));
    }
    return array;
}

dataformats::json::Value to_json_value(PlatformStyle platform) {
    return dataformats::json::Value(platform_label(platform));
}

dataformats::json::Value to_json_value(PanelKind kind) {
    switch (kind) {
        case PanelKind::Generic:
            return dataformats::json::Value("generic");
        case PanelKind::Chart:
            return dataformats::json::Value("chart");
        case PanelKind::CadViewport:
            return dataformats::json::Value("cadViewport");
        case PanelKind::ComponentGroup:
            return dataformats::json::Value("componentGroup");
    }
    return dataformats::json::Value("generic");
}

dataformats::json::Object to_json_object(const ::graphics::components::Component& component) {
    dataformats::json::Object object;
    object.set("type", dataformats::json::Value(::graphics::components::component_type_name(component.type())));
    object.set("label", dataformats::json::Value(component.label()));
    object.set("text", dataformats::json::Value(component.text()));
    object.set("enabled", dataformats::json::Value(component.enabled()));
    object.set("selected", dataformats::json::Value(component.selected()));
    object.set("checked", dataformats::json::Value(component.checked()));
    object.set("pressed", dataformats::json::Value(component.pressed()));
    object.set("focused", dataformats::json::Value(component.focused()));
    object.set("width", dataformats::json::Value(static_cast<int>(component.width())));
    object.set("cursorPosition", dataformats::json::Value(static_cast<int>(component.cursor_position())));
    object.set("rendered", dataformats::json::Value(to_json_array(component.render())));

    dataformats::json::Object layout_group;
    if (const auto* holder = component.layout_group()) {
        layout_group.set("present", dataformats::json::Value(true));
        layout_group.set("layout", dataformats::json::Value(::graphics::components::layout_type_name(holder->layout())));
        layout_group.set("columns", dataformats::json::Value(static_cast<int>(holder->columns())));

        dataformats::json::Array nested_components;
        for (const auto& nested : holder->components()) {
            nested_components.push(dataformats::json::Value(to_json_object(nested)));
        }
        layout_group.set("components", dataformats::json::Value(nested_components));
    } else {
        layout_group.set("present", dataformats::json::Value(false));
        layout_group.set("layout", dataformats::json::Value(""));
        layout_group.set("columns", dataformats::json::Value(0));
        layout_group.set("components", dataformats::json::Value(dataformats::json::Array{}));
    }
    object.set("layoutGroup", dataformats::json::Value(layout_group));
    return object;
}

dataformats::json::Object to_json_object(const MenuItem& item) {
    dataformats::json::Object object;
    object.set("label", dataformats::json::Value(item.label));
    object.set("shortcut", dataformats::json::Value(item.shortcut));
    object.set("enabled", dataformats::json::Value(item.enabled));
    object.set("separator", dataformats::json::Value(item.separator));
    object.set("checked", dataformats::json::Value(item.checked));
    return object;
}

dataformats::json::Object to_json_object(const Menu& menu) {
    dataformats::json::Object object;
    object.set("title", dataformats::json::Value(menu.title));
    dataformats::json::Array items;
    for (const auto& item : menu.items) {
        items.push(dataformats::json::Value(to_json_object(item)));
    }
    object.set("items", dataformats::json::Value(items));
    return object;
}

dataformats::json::Object to_json_object(const CadViewport& viewport, bool present) {
    dataformats::json::Object object;
    object.set("present", dataformats::json::Value(present));
    object.set("modelName", dataformats::json::Value(viewport.model_name));
    object.set("projection", dataformats::json::Value(viewport.projection));
    object.set("layers", dataformats::json::Value(to_json_array(viewport.layers)));
    object.set("showGrid", dataformats::json::Value(viewport.show_grid));
    object.set("showAxes", dataformats::json::Value(viewport.show_axes));
    object.set("primitiveCount", dataformats::json::Value(static_cast<int>(viewport.primitive_count)));
    return object;
}

dataformats::json::Object to_json_object(const Panel& panel) {
    dataformats::json::Object object;
    object.set("kind", to_json_value(panel.kind));
    object.set("title", dataformats::json::Value(panel.title));
    object.set("preferredHeight", dataformats::json::Value(static_cast<int>(panel.preferred_height)));
    object.set("bordered", dataformats::json::Value(panel.bordered));
    object.set("lines", dataformats::json::Value(to_json_array(panel.lines)));

    dataformats::json::Object embedded_chart;
    embedded_chart.set("width", dataformats::json::Value(static_cast<int>(panel.embedded_chart_width)));
    embedded_chart.set("height", dataformats::json::Value(static_cast<int>(panel.embedded_chart_height)));
    embedded_chart.set("preview", dataformats::json::Value(to_json_array(panel.embedded_chart_preview)));
    object.set("embeddedChart", dataformats::json::Value(embedded_chart));

    dataformats::json::Array components;
    for (const auto& component : panel.embedded_components) {
        components.push(dataformats::json::Value(to_json_object(component)));
    }
    object.set("components", dataformats::json::Value(components));
    object.set("componentLayout", dataformats::json::Value(panel.embedded_component_layout));
    object.set("componentColumns", dataformats::json::Value(static_cast<int>(panel.embedded_component_columns)));

    object.set("cadViewport", dataformats::json::Value(to_json_object(panel.cad_viewport_state, panel.has_cad_viewport)));
    return object;
}

std::vector<std::string> canvas_preview_lines(const ::graphics::Canvas& canvas,
                                              std::size_t preview_width = 24,
                                              std::size_t preview_height = 6) {
    if (canvas.width() <= 0 || canvas.height() <= 0 || preview_width == 0 || preview_height == 0) {
        return {};
    }

    static const std::string density = " .:-=+*#%@";
    std::vector<std::string> lines;
    lines.reserve(preview_height);

    for (std::size_t y = 0; y < preview_height; ++y) {
        std::string row;
        row.reserve(preview_width);
        for (std::size_t x = 0; x < preview_width; ++x) {
            const int source_x = static_cast<int>((x * static_cast<std::size_t>(canvas.width())) / preview_width);
            const int source_y = static_cast<int>((y * static_cast<std::size_t>(canvas.height())) / preview_height);
            const ::graphics::Color color = canvas.get_pixel(std::min(source_x, canvas.width() - 1),
                                                             std::min(source_y, canvas.height() - 1));
            const std::uint16_t brightness = static_cast<std::uint16_t>(color.r) +
                                             static_cast<std::uint16_t>(color.g) +
                                             static_cast<std::uint16_t>(color.b);
            const std::size_t index = std::min<std::size_t>((brightness * density.size()) / (256U * 3U), density.size() - 1);
            row.push_back(density[index]);
        }
        lines.push_back(row);
    }

    return lines;
}

std::vector<std::string> fit_lines(const std::vector<std::string>& lines, std::size_t width, std::size_t limit) {
    std::vector<std::string> fitted;
    fitted.reserve(std::min(lines.size(), limit));
    for (std::size_t i = 0; i < lines.size() && fitted.size() < limit; ++i) {
        fitted.push_back(truncate_or_pad(lines[i], width));
    }
    while (fitted.size() < limit) {
        fitted.push_back(std::string(width, ' '));
    }
    return fitted;
}

std::vector<std::string> render_panel_lines(const Panel& panel, std::size_t width, PlatformStyle platform) {
    if (width < 4) {
        return {};
    }

    const std::size_t inner_width = panel.bordered ? width - 2 : width;
    const std::size_t desired_height = std::max<std::size_t>(panel.preferred_height, panel.bordered ? 3 : 1);
    std::vector<std::string> lines;

    if (panel.bordered) {
        lines.push_back("[" + truncate_or_pad(panel.title, inner_width) + "]");
        const std::size_t body_rows = desired_height - 2;
        const auto body = fit_lines(panel.lines, inner_width, body_rows);
        for (const auto& line : body) {
            lines.push_back("|" + line + "|");
        }
        lines.push_back("+" + repeat('-', inner_width) + "+");
    } else {
        lines.push_back(truncate_or_pad(panel.title, inner_width));
        const auto body = fit_lines(panel.lines, inner_width, desired_height - 1);
        lines.insert(lines.end(), body.begin(), body.end());
    }

    if (panel.kind == PanelKind::Chart && !lines.empty()) {
        lines[0] = truncate_or_pad("[Chart] " + panel.title, width);
    } else if (panel.kind == PanelKind::CadViewport && !lines.empty()) {
        lines[0] = truncate_or_pad("[3D CAD " + platform_label(platform) + "] " + panel.title, width);
    } else if (panel.kind == PanelKind::ComponentGroup && !lines.empty()) {
        lines[0] = truncate_or_pad("[Components] " + panel.title, width);
    }
    return lines;
}

std::string platform_controls(PlatformStyle platform) {
    switch (platform) {
        case PlatformStyle::MacOS: return "● ● ●";
        case PlatformStyle::Linux: return "◻ ◇ ✕";
        case PlatformStyle::Windows: return "— □ ✕";
    }
    return "□ □ □";
}

std::string format_shortcut(PlatformStyle platform, const std::string& shortcut) {
    if (shortcut.empty()) {
        return "";
    }

    if (platform == PlatformStyle::MacOS) {
        std::string formatted = shortcut;
        const std::pair<const char*, const char*> replacements[] = {
            {"Ctrl+", "⌘"},
            {"Alt+", "⌥"},
            {"Shift+", "⇧"},
        };
        for (const auto& replacement : replacements) {
            std::size_t pos = 0;
            while ((pos = formatted.find(replacement.first, pos)) != std::string::npos) {
                formatted.replace(pos, std::string(replacement.first).size(), replacement.second);
                pos += std::string(replacement.second).size();
            }
        }
        return formatted;
    }

    return shortcut;
}

} // namespace

MenuItem MenuItem::action(std::string label, std::string shortcut, bool enabled, bool checked) {
    MenuItem item;
    item.label = std::move(label);
    item.shortcut = std::move(shortcut);
    item.enabled = enabled;
    item.checked = checked;
    return item;
}

MenuItem MenuItem::divider() {
    MenuItem item;
    item.separator = true;
    item.enabled = false;
    return item;
}

Menu::Menu(std::string menu_title) : title(std::move(menu_title)) {}

Menu& Menu::add_item(const MenuItem& item) {
    items.push_back(item);
    return *this;
}

CadViewport::CadViewport(std::string name) : model_name(std::move(name)) {}

Panel Panel::generic(std::string title,
                     std::vector<std::string> lines,
                     std::size_t preferred_height,
                     bool bordered) {
    Panel panel;
    panel.kind = PanelKind::Generic;
    panel.title = std::move(title);
    panel.lines = std::move(lines);
    panel.preferred_height = preferred_height;
    panel.bordered = bordered;
    return panel;
}

Panel Panel::chart_preview(std::string title,
                           const ::graphics::Canvas& canvas,
                           std::vector<std::string> summary_lines) {
    Panel panel;
    panel.kind = PanelKind::Chart;
    panel.title = std::move(title);
    panel.preferred_height = 10;
    panel.bordered = true;
    panel.embedded_chart_width = static_cast<std::size_t>(std::max(canvas.width(), 0));
    panel.embedded_chart_height = static_cast<std::size_t>(std::max(canvas.height(), 0));
    panel.embedded_chart_preview = canvas_preview_lines(canvas);
    panel.lines.push_back("Canvas: " + std::to_string(canvas.width()) + "x" + std::to_string(canvas.height()));
    panel.lines.push_back("Embedded chart preview:");
    panel.lines.insert(panel.lines.end(), panel.embedded_chart_preview.begin(), panel.embedded_chart_preview.end());
    panel.lines.insert(panel.lines.end(), summary_lines.begin(), summary_lines.end());
    panel.preferred_height = std::max<std::size_t>(panel.preferred_height, panel.lines.size() + 2);
    return panel;
}

Panel Panel::chart_preview(std::string title,
                           const ::graphics::Graph& graph,
                           std::vector<std::string> summary_lines) {
    auto rendered = graph.render();
    summary_lines.insert(summary_lines.begin(), "Source: graphics::Graph");
    return Panel::chart_preview(std::move(title), rendered, std::move(summary_lines));
}

Panel Panel::component_group(std::string title,
                             const std::vector<::graphics::components::Component>& components,
                             std::size_t preferred_height,
                             bool bordered) {
    Panel panel;
    panel.kind = PanelKind::ComponentGroup;
    panel.title = std::move(title);
    panel.bordered = bordered;
    panel.embedded_components = components;

    for (std::size_t i = 0; i < components.size(); ++i) {
        const auto rendered = components[i].render();
        panel.lines.insert(panel.lines.end(), rendered.begin(), rendered.end());
        if (i + 1 < components.size()) {
            panel.lines.push_back("");
        }
    }

    panel.preferred_height = preferred_height == 0
        ? std::max<std::size_t>(panel.lines.size() + (bordered ? 2 : 1), 4)
        : preferred_height;
    return panel;
}

Panel Panel::component_group(std::string title,
                             const ::graphics::components::ComponentHolder& holder,
                             std::size_t preferred_height,
                             bool bordered) {
    Panel panel;
    panel.kind = PanelKind::ComponentGroup;
    panel.title = std::move(title);
    panel.bordered = bordered;
    panel.embedded_components = holder.components();
    panel.embedded_component_layout = ::graphics::components::layout_type_name(holder.layout());
    panel.embedded_component_columns = holder.columns();
    panel.lines = holder.render();
    panel.preferred_height = preferred_height == 0
        ? std::max<std::size_t>(panel.lines.size() + (bordered ? 2 : 1), 4)
        : preferred_height;
    return panel;
}

Panel Panel::cad_viewport(const CadViewport& viewport, std::size_t preferred_height) {
    Panel panel;
    panel.kind = PanelKind::CadViewport;
    panel.title = viewport.model_name.empty() ? "CAD Viewport" : viewport.model_name;
    panel.preferred_height = preferred_height;
    panel.bordered = true;
    panel.has_cad_viewport = true;
    panel.cad_viewport_state = viewport;
    panel.lines.push_back("Projection: " + viewport.projection);
    panel.lines.push_back(std::string("Grid: ") + (viewport.show_grid ? "on" : "off") +
                          " | Axes: " + (viewport.show_axes ? "on" : "off"));
    panel.lines.push_back("Primitives: " + std::to_string(viewport.primitive_count));
    if (!viewport.layers.empty()) {
        panel.lines.push_back("Layers: " + viewport.layers.front());
        for (std::size_t i = 1; i < viewport.layers.size(); ++i) {
            panel.lines.push_back("        " + viewport.layers[i]);
        }
    }
    panel.lines.push_back("Viewport ready for CAD scene composition");
    return panel;
}

WindowSimulator::WindowSimulator(std::string window_title,
                                 std::size_t width,
                                 std::size_t height,
                                 PlatformStyle platform)
    : title_(std::move(window_title))
    , width_(std::max<std::size_t>(width, 24))
    , height_(std::max<std::size_t>(height, 8))
    , platform_(platform) {}

WindowSimulator& WindowSimulator::set_title(const std::string& title) {
    title_ = title;
    return *this;
}

WindowSimulator& WindowSimulator::set_size(std::size_t width, std::size_t height) {
    width_ = std::max<std::size_t>(width, 24);
    height_ = std::max<std::size_t>(height, 8);
    return *this;
}

WindowSimulator& WindowSimulator::set_platform(PlatformStyle platform) {
    platform_ = platform;
    return *this;
}

WindowSimulator& WindowSimulator::set_status_text(const std::string& status_text) {
    status_text_ = status_text;
    return *this;
}

WindowSimulator& WindowSimulator::set_content(const std::vector<std::string>& content_lines) {
    content_lines_ = content_lines;
    return *this;
}

WindowSimulator& WindowSimulator::add_menu(const Menu& menu) {
    menus_.push_back(menu);
    return *this;
}

WindowSimulator& WindowSimulator::clear_menus() {
    menus_.clear();
    return *this;
}

WindowSimulator& WindowSimulator::add_panel(const Panel& panel) {
    panels_.push_back(panel);
    return *this;
}

WindowSimulator& WindowSimulator::clear_panels() {
    panels_.clear();
    return *this;
}

std::string WindowSimulator::platform_name() const {
    return platform_label(platform_);
}

std::string WindowSimulator::render() const {
    const std::size_t inner_width = width_ - 2;
    const std::size_t content_rows = height_ >= 5 ? height_ - 5 : 0;
    const std::string top_border = "+" + repeat('-', inner_width) + "+";

    std::ostringstream stream;
    stream << top_border << '\n';

    const std::string title_bar = platform_controls(platform_) + "  " + title_ + " [" + platform_name() + "]";
    stream << "|" << truncate_or_pad(title_bar, inner_width) << "|\n";

    const std::string menu_bar = menus_.empty() ? "<no menus configured>" : join_menu_titles(menus_);
    stream << "|" << truncate_or_pad(menu_bar, inner_width) << "|\n";
    stream << "|" << truncate_or_pad(repeat('=', std::min<std::size_t>(inner_width, std::max<std::size_t>(1, inner_width))), inner_width) << "|\n";

    std::vector<std::string> body_lines;
    if (!panels_.empty()) {
        for (const auto& panel : panels_) {
            const auto panel_lines = render_panel_lines(panel, inner_width, platform_);
            body_lines.insert(body_lines.end(), panel_lines.begin(), panel_lines.end());
        }
    } else {
        body_lines = content_lines_;
    }

    for (std::size_t row = 0; row < content_rows; ++row) {
        const std::string line = row < body_lines.size() ? body_lines[row] : "";
        stream << "|" << truncate_or_pad(line, inner_width) << "|\n";
    }

    stream << "|" << truncate_or_pad(status_text_.empty() ? "Ready" : status_text_, inner_width) << "|\n";
    stream << top_border;
    return stream.str();
}

std::string WindowSimulator::render_menu_dropdown(const std::string& menu_title) const {
    const auto menu_it = std::find_if(menus_.begin(), menus_.end(), [&](const Menu& menu) {
        return menu.title == menu_title;
    });
    if (menu_it == menus_.end()) {
        throw std::invalid_argument("Unknown menu title: " + menu_title);
    }

    std::size_t width = menu_title.size() + 4;
    for (const auto& item : menu_it->items) {
        if (item.separator) {
            continue;
        }
        std::ostringstream row;
        row << (item.checked ? "✓ " : "  ") << item.label;
        const std::string shortcut = format_shortcut(platform_, item.shortcut);
        if (!shortcut.empty()) {
            row << std::string(4, ' ') << shortcut;
        }
        std::string text = row.str();
        if (!item.enabled) {
            text = "(" + text + ")";
        }
        width = std::max(width, text.size());
    }

    std::ostringstream stream;
    stream << "/" << repeat('-', width) << "\\\n";
    for (const auto& item : menu_it->items) {
        if (item.separator) {
            stream << "|" << repeat('-', width) << "|\n";
            continue;
        }

        std::ostringstream row;
        row << (item.checked ? "✓ " : "  ") << item.label;
        const std::string shortcut = format_shortcut(platform_, item.shortcut);
        const std::size_t available_width = item.enabled ? width : (width >= 2 ? width - 2 : 0);
        if (!shortcut.empty()) {
            const std::size_t used = row.str().size();
            if (available_width > used + shortcut.size()) {
                row << std::string(available_width - used - shortcut.size(), ' ');
            }
            row << shortcut;
        }
        std::string text = row.str();
        if (!item.enabled) {
            text = "(" + text + ")";
        }
        stream << "|" << truncate_or_pad(text, width) << "|\n";
    }
    stream << "\\" << repeat('-', width) << "/";
    return stream.str();
}

std::string WindowSimulator::to_json() const {
    dataformats::json::Object root;
    root.set("title", dataformats::json::Value(title_));
    root.set("width", dataformats::json::Value(static_cast<int>(width_)));
    root.set("height", dataformats::json::Value(static_cast<int>(height_)));
    root.set("platform", to_json_value(platform_));
    root.set("statusText", dataformats::json::Value(status_text_));
    root.set("contentLines", dataformats::json::Value(to_json_array(content_lines_)));

    dataformats::json::Array menus_array;
    for (const auto& menu : menus_) {
        menus_array.push(dataformats::json::Value(to_json_object(menu)));
    }
    root.set("menus", dataformats::json::Value(menus_array));

    dataformats::json::Array panels_array;
    for (const auto& panel : panels_) {
        panels_array.push(dataformats::json::Value(to_json_object(panel)));
    }
    root.set("panels", dataformats::json::Value(panels_array));

    return root.to_string();
}

bool WindowSimulator::save_json(const std::string& path) const {
    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }
    output << to_json();
    return output.good();
}

} // namespace windows
} // namespace graphics
