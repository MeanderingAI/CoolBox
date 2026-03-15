#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../../charts/headers/graphics.h"

namespace graphics {
namespace windows {

enum class PlatformStyle {
    MacOS,
    Linux,
    Windows
};

struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    bool checked = false;

    static MenuItem action(std::string label,
                           std::string shortcut = "",
                           bool enabled = true,
                           bool checked = false);
    static MenuItem divider();
};

struct Menu {
    std::string title;
    std::vector<MenuItem> items;

    Menu() = default;
    explicit Menu(std::string title);

    Menu& add_item(const MenuItem& item);
};

enum class PanelKind {
    Generic,
    Chart,
    CadViewport
};

struct CadViewport {
    std::string model_name;
    std::string projection = "Perspective";
    std::vector<std::string> layers;
    bool show_grid = true;
    bool show_axes = true;
    std::uint32_t primitive_count = 0;

    CadViewport() = default;
    explicit CadViewport(std::string model_name);
};

struct Panel {
    PanelKind kind = PanelKind::Generic;
    std::string title;
    std::vector<std::string> lines;
    std::size_t preferred_height = 6;
    bool bordered = true;
    std::size_t embedded_chart_width = 0;
    std::size_t embedded_chart_height = 0;
    std::vector<std::string> embedded_chart_preview;
    bool has_cad_viewport = false;
    CadViewport cad_viewport_state;

    static Panel generic(std::string title,
                         std::vector<std::string> lines = {},
                         std::size_t preferred_height = 6,
                         bool bordered = true);
    static Panel chart_preview(std::string title,
                               const ::graphics::Canvas& canvas,
                               std::vector<std::string> summary_lines = {});
    static Panel chart_preview(std::string title,
                               const ::graphics::Graph& graph,
                               std::vector<std::string> summary_lines = {});
    static Panel cad_viewport(const CadViewport& viewport,
                              std::size_t preferred_height = 8);
};

class WindowSimulator {
public:
    WindowSimulator(std::string title = "Untitled Window",
                    std::size_t width = 80,
                    std::size_t height = 24,
                    PlatformStyle platform = PlatformStyle::MacOS);

    WindowSimulator& set_title(const std::string& title);
    WindowSimulator& set_size(std::size_t width, std::size_t height);
    WindowSimulator& set_platform(PlatformStyle platform);
    WindowSimulator& set_status_text(const std::string& status_text);
    WindowSimulator& set_content(const std::vector<std::string>& content_lines);
    WindowSimulator& add_menu(const Menu& menu);
    WindowSimulator& clear_menus();
    WindowSimulator& add_panel(const Panel& panel);
    WindowSimulator& clear_panels();

    const std::string& title() const { return title_; }
    std::size_t width() const { return width_; }
    std::size_t height() const { return height_; }
    PlatformStyle platform() const { return platform_; }
    const std::vector<Menu>& menus() const { return menus_; }
    const std::vector<Panel>& panels() const { return panels_; }
    const std::vector<std::string>& content_lines() const { return content_lines_; }
    const std::string& status_text() const { return status_text_; }

    std::string render() const;
    std::string render_menu_dropdown(const std::string& menu_title) const;
    std::string platform_name() const;
    std::string to_json() const;
    bool save_json(const std::string& path) const;

private:
    std::string title_;
    std::size_t width_;
    std::size_t height_;
    PlatformStyle platform_;
    std::vector<Menu> menus_;
    std::vector<Panel> panels_;
    std::vector<std::string> content_lines_;
    std::string status_text_;
};

} // namespace windows
} // namespace graphics
