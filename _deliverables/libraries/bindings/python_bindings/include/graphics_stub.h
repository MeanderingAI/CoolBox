#ifndef COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_STUB_H
#define COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_STUB_H

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace graphics {

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
};

namespace Colors {
    extern const Color Black;
    extern const Color White;
    extern const Color Red;
    extern const Color Green;
    extern const Color Blue;
    extern const Color Orange;
    extern const Color Purple;
    extern const Color Cyan;
    extern const Color Gray;
    extern const Color DarkGray;
    extern const Color LightGray;
}

class Canvas {
public:
    int width() const;
    int height() const;
    const uint8_t* data() const;
};

enum class GraphType { Line, Bar, Scatter };

struct DataSeries {
    std::string label;
    std::vector<double> x_values;
    std::vector<double> y_values;
    Color color;
};

class Graph {
public:
    Graph(int w, int h, GraphType t);
    void set_title(const std::string& title);
    void set_x_label(const std::string& label);
    void set_y_label(const std::string& label);
    void set_type(GraphType t);
    void add_series(const DataSeries& s);
    Canvas render() const;
};

class Table {
public:
    Table();
    void set_headers(const std::vector<std::string>& headers);
    void add_row(const std::vector<std::string>& row);
    Canvas render() const;
    void set_cell_padding(int px);
    void set_font_scale(int scale);
    void set_header_color(Color c);
    void set_border_color(Color c);
    void set_alternate_row_color(Color c);
};

namespace components {

struct ToolbarModel {
    std::vector<std::string> actions;
    std::size_t spacing;
    void add_action(const std::string&);
    void set_spacing(std::size_t);
};

struct DockPanelModel {
    std::string title;
    bool floating = false;
    void set_floating(bool);
};

struct LayerListModel {
    std::vector<std::string> layers;
    std::size_t selected = 0;
    void add_layer(const std::string&);
    void set_selected(std::size_t);
};

struct PropertyInspectorModel {
    std::vector<std::pair<std::string,std::string>> properties;
    void add_property(const std::pair<std::string,std::string>&);
};

struct FileTreeModel {
    struct Node {
        std::string name;
        bool is_dir = false;
        std::vector<Node> children;
    };
    Node root;
};

struct RadioSelectorModel {
    std::vector<std::string> options;
    std::size_t selected = 0;
    void add_option(const std::string&);
    void set_selected(std::size_t);
};

struct CheckboxGroupModel {
    std::vector<std::string> options;
    std::vector<bool> checked;
    void add_option(const std::string&);
    void set_checked(std::size_t, bool);
};

} // namespace components

} // namespace graphics
#endif  // COOLBOX__LIBRARIES_PYTHON_BINDINGS_INCLUDE_GRAPHICS_STUB_H
