#ifndef COOLBOX_GRAPHICS_PLOTTING_HPP
#define COOLBOX_GRAPHICS_PLOTTING_HPP

#include "full_application_window.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace graphics {

struct Plot2DOptions {
    int width = 800;
    int height = 600;
    std::string title;
    std::string x_label = "X";
    std::string y_label = "Y";
    std::string series_label = "Series";
    Color color = Colors::Blue;
    GraphType type = GraphType::Line;
    bool show_window = false;
    std::string save_path = "plot.png";
};

struct Plot3DOptions {
    int width = 800;
    int height = 600;
    std::string title;
    std::string x_label = "X";
    std::string y_label = "Y";
    std::string z_label = "Z";
    std::string series_label = "Series";
    Color color = Colors::Blue;
    Graph3DType type = Graph3DType::Scatter;
    double azimuth_degrees = 45.0;
    double elevation_degrees = 25.0;
    bool show_window = false;
    std::string save_path = "plot.png";
};

inline Canvas plot_2d(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const Plot2DOptions& options = {}) {
    if (x.empty() || x.size() != y.size()) {
        throw std::invalid_argument("plot_2d: x and y need equal, non-empty lengths");
    }

    Graph graph(options.width, options.height, options.type);
    graph.set_title(options.title);
    graph.set_x_label(options.x_label);
    graph.set_y_label(options.y_label);
    graph.add_series({options.series_label, x, y, options.color});
    Canvas canvas = graph.render();
    if (options.show_window) {
        full_application_window::show_canvas_window(
            canvas, options.title, options.save_path);
    }
    return canvas;
}

inline Canvas plot_3d(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      const Plot3DOptions& options = {}) {
    if (x.empty() || x.size() != y.size() || x.size() != z.size()) {
        throw std::invalid_argument(
            "plot_3d: x, y, and z need equal, non-empty lengths");
    }
    if (!std::isfinite(options.azimuth_degrees) ||
        !std::isfinite(options.elevation_degrees)) {
        throw std::invalid_argument("plot_3d: view angles must be finite");
    }

    Graph3D graph(options.width, options.height, options.type);
    graph.set_title(options.title);
    graph.set_x_label(options.x_label);
    graph.set_y_label(options.y_label);
    graph.set_z_label(options.z_label);
    graph.set_view(options.azimuth_degrees, options.elevation_degrees);
    graph.add_series({options.series_label, x, y, z, options.color});
    Canvas canvas = graph.render();
    if (options.show_window) {
        full_application_window::show_canvas_window(
            canvas, options.title, options.save_path);
    }
    return canvas;
}

} // namespace graphics

#endif
