/**
 * @file graphics.h
 * @brief Lightweight charting & table-rendering library with image export.
 *
 * Provides a pixel-based Canvas with drawing primitives, plus higher-level
 * Graph and Table renderers that produce publication-ready images.
 *
 * Export formats:
 *   - BMP  (built-in, zero dependencies)
 *   - PNG  (via stb_image_write, fetched by CMake)
 *   - JPG  (via stb_image_write, fetched by CMake)
 *
 * Usage:
 * @code{.cpp}
 * using namespace graphics;
 *
 * // --- Line graph ---
 * Graph g(800, 600, GraphType::Line);
 * g.set_title("Training Loss");
 * g.set_x_label("Epoch");
 * g.set_y_label("Loss");
 * g.add_series({"train", {1,2,3,4,5}, {0.9,0.7,0.4,0.2,0.1}, Colors::Blue});
 * g.add_series({"valid", {1,2,3,4,5}, {0.95,0.8,0.6,0.4,0.3}, Colors::Red});
 * Canvas c = g.render();
 * c.save_png("loss.png");
 *
 * // --- Bar chart ---
 * Graph bar(600, 400, GraphType::Bar);
 * bar.set_title("Scores");
 * bar.add_series({"scores", {1,2,3}, {85,92,78}, Colors::Green});
 * bar.render().save_jpg("scores.jpg", 90);
 *
 * // --- Table ---
 * Table t;
 * t.set_headers({"Name", "Score", "Grade"});
 * t.add_row({"Alice", "95", "A"});
 * t.add_row({"Bob",   "82", "B"});
 * t.render().save_bmp("grades.bmp");
 * @endcode
 */
#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_CHARTS_HEADERS_GRAPHICS_H
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_CHARTS_HEADERS_GRAPHICS_H
/**
 * @file graphics.h
 * @brief Lightweight charting & table-rendering library with image export.
 *
 * Provides a pixel-based Canvas with drawing primitives, plus higher-level
 * Graph and Table renderers that produce publication-ready images.
 *
 * Export formats:
 *   - BMP  (built-in, zero dependencies)
 *   - PNG  (via stb_image_write, fetched by CMake)
 *   - JPG  (via stb_image_write, fetched by CMake)
 *
 * Usage:
 * @code{.cpp}
 * using namespace graphics;
 *
 * // --- Line graph ---
 * Graph g(800, 600, GraphType::Line);
 * g.set_title("Training Loss");
 * g.set_x_label("Epoch");
 * g.set_y_label("Loss");
 * g.add_series({"train", {1,2,3,4,5}, {0.9,0.7,0.4,0.2,0.1}, Colors::Blue});
 * g.add_series({"valid", {1,2,3,4,5}, {0.95,0.8,0.6,0.4,0.3}, Colors::Red});
 * Canvas c = g.render();
 * c.save_png("loss.png");
 *
 * // --- Bar chart ---
 * Graph bar(600, 400, GraphType::Bar);
 * bar.set_title("Scores");
 * bar.add_series({"scores", {1,2,3}, {85,92,78}, Colors::Green});
 * bar.render().save_jpg("scores.jpg", 90);
 *
 * // --- Table ---
 * Table t;
 * t.set_headers({"Name", "Score", "Grade"});
 * t.add_row({"Alice", "95", "A"});
 * t.add_row({"Bob",   "82", "B"});
 * t.render().save_bmp("grades.bmp");
 * @endcode
 */
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace graphics {

// Color and Canvas definitions
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

namespace Colors {
    inline constexpr Color Black   {  0,   0,   0, 255};
    inline constexpr Color White   {255, 255, 255, 255};
    inline constexpr Color Red     {220,  50,  50, 255};
    inline constexpr Color Green   { 50, 180,  50, 255};
    inline constexpr Color Blue    { 50,  90, 220, 255};
    inline constexpr Color Orange  {230, 150,  30, 255};
    inline constexpr Color Purple  {150,  50, 200, 255};
    inline constexpr Color Cyan    { 50, 200, 200, 255};
    inline constexpr Color Gray    {180, 180, 180, 255};
    inline constexpr Color DarkGray{100, 100, 100, 255};
    inline constexpr Color LightGray{230, 230, 230, 255};
}

// Canvas – pixel buffer with drawing primitives
class Canvas {
public:
    Canvas(int width, int height, Color bg = Colors::White);

    // Accessors
    int width()  const { return width_; }
    int height() const { return height_; }
    const uint8_t* data() const { return pixels_.data(); }

    // Pixel operations
    void set_pixel(int x, int y, Color c);
    Color get_pixel(int x, int y) const;
    void fill(Color c);

    // Drawing primitives
    void draw_line(int x0, int y0, int x1, int y1, Color c, int thickness = 1);
    void draw_rect(int x, int y, int w, int h, Color c, bool filled = false);
    void draw_circle(int cx, int cy, int radius, Color c, bool filled = false);

    // Polygon / plane drawing
    enum class FillStyle { Solid, VerticalGradient, Hatch };
    void draw_polygon(const std::vector<std::pair<int,int>>& pts,
                      Color outline,
                      bool filled = false,
                      FillStyle style = FillStyle::Solid,
                      Color fill_color = Colors::LightGray,
                      Color fill_color2 = Colors::White,
                      int hatch_spacing = 6);

    void draw_text(int x, int y, const std::string& text, Color c, int scale = 1);

    // Export
    bool save_bmp(const std::string& path) const;
    bool save_png(const std::string& path) const;
    bool save_jpg(const std::string& path, int quality = 90) const;

private:
    int width_, height_;
    std::vector<uint8_t> pixels_;   // RGBA, row-major, top-left origin

    void blend_pixel(int x, int y, Color c);
};

// ===================================================================
// Color
// ===================================================================

// ===================================================================
// Fractal and Plotting Primitives (NEW)

// Fractal types
enum class FractalType { Mandelbrot, Julia };

class Fractal {
public:
    Fractal(int width, int height, FractalType type);

    void set_params(double param1, double param2 = 0.0); // e.g., Julia c = (param1, param2)
    void set_max_iter(int max_iter);
    void set_bounds(double x_min, double x_max, double y_min, double y_max);
    Canvas render() const;
private:
    int width_, height_, max_iter_ = 1000;
    FractalType type_;
    double param1_ = 0.0, param2_ = 0.0;
    double x_min_ = -2.0, x_max_ = 2.0, y_min_ = -2.0, y_max_ = 2.0;
};

// Function plotter (y = f(x))
class FunctionPlot {
public:
    FunctionPlot(int width, int height);
    void set_equation(const std::string& expr); // e.g., "sin(x) + x^2"
    void set_range(double x_min, double x_max);
    void set_samples(int n);
    void set_color(Color c);
    Canvas render() const;
private:
    int width_, height_, samples_ = 500;
    std::string expr_;
    double x_min_ = -10.0, x_max_ = 10.0;
    Color color_ = Colors::Blue;
};

// Parametric plotter (x = f(t), y = g(t))
class ParametricPlot {
public:
    ParametricPlot(int width, int height);
    void set_equations(const std::string& x_expr, const std::string& y_expr); // e.g., "cos(t)", "sin(t)"
    void set_t_range(double t_min, double t_max);
    void set_samples(int n);
    void set_color(Color c);
    Canvas render() const;
private:
    int width_, height_, samples_ = 500;
    std::string x_expr_, y_expr_;
    double t_min_ = 0.0, t_max_ = 2 * 3.141592653589793;
    Color color_ = Colors::Red;
};

// Polar plotter (r = f(theta))
class PolarPlot {
public:
    PolarPlot(int width, int height);
    void set_equation(const std::string& expr); // e.g., "1 + sin(5*theta)"
    void set_theta_range(double theta_min, double theta_max);
    void set_samples(int n);
    void set_color(Color c);
    Canvas render() const;
private:
    int width_, height_, samples_ = 500;
    std::string expr_;
    double theta_min_ = 0.0, theta_max_ = 2 * 3.141592653589793;
    Color color_ = Colors::Purple;
};

// Histogram
class HistogramPlot {
public:
    HistogramPlot(int width, int height);
    void set_data(const std::vector<double>& values);
    void set_bins(int n);
    void set_color(Color c);
    Canvas render() const;
private:
    int width_, height_, bins_ = 20;
    std::vector<double> values_;
    Color color_ = Colors::Orange;
};

// (moved Color/Colors earlier to make them visible to types that use them)

// ===================================================================
// Graph
// ===================================================================

enum class GraphType { Line, Bar, Scatter };

struct DataSeries {
    std::string label;
    std::vector<double> x_values;
    std::vector<double> y_values;
    Color color = Colors::Blue;
};

class Graph {
public:
    explicit Graph(int width = 800, int height = 600,
                   GraphType type = GraphType::Line);

    void set_title(const std::string& title);
    void set_x_label(const std::string& label);
    void set_y_label(const std::string& label);
    void set_type(GraphType type);
    void add_series(const DataSeries& series);

    /** Render the graph and return the resulting canvas. */
    Canvas render() const;

private:
    int width_, height_;
    GraphType type_;
    std::string title_;
    std::string x_label_, y_label_;
    std::vector<DataSeries> series_;

    // Internal layout
    struct PlotArea { int x, y, w, h; };
    PlotArea compute_plot_area() const;

    void draw_axes(Canvas& canvas, const PlotArea& area,
                   double x_min, double x_max,
                   double y_min, double y_max) const;
    void draw_line_series(Canvas& canvas, const PlotArea& area,
                          const DataSeries& s,
                          double x_min, double x_max,
                          double y_min, double y_max) const;
    void draw_bar_series(Canvas& canvas, const PlotArea& area,
                         int series_idx, int total_series,
                         const DataSeries& s,
                         double x_min, double x_max,
                         double y_min, double y_max) const;
    void draw_scatter_series(Canvas& canvas, const PlotArea& area,
                             const DataSeries& s,
                             double x_min, double x_max,
                             double y_min, double y_max) const;
    void draw_legend(Canvas& canvas, const PlotArea& area) const;
};

// ===================================================================
// Table
// ===================================================================

class Table {
public:
    Table();

    void set_headers(const std::vector<std::string>& headers);
    void add_row(const std::vector<std::string>& row);

    void set_cell_padding(int px);
    void set_font_scale(int scale);
    void set_header_color(Color c);
    void set_border_color(Color c);
    void set_alternate_row_color(Color c);

    /** Render the table and return the resulting canvas. */
    Canvas render() const;

private:
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    int cell_padding_ = 8;
    int font_scale_ = 1;
    Color header_bg_    = {70, 130, 200, 255};
    Color header_fg_    = Colors::White;
    Color border_color_ = Colors::Gray;
    Color alt_row_color_= {245, 245, 255, 255};

    int compute_col_width(int col) const;
};

} // namespace graphics
#endif  // COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_CHARTS_HEADERS_GRAPHICS_H
