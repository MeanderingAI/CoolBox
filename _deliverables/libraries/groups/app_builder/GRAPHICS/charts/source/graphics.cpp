#include "../headers/graphics.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <iomanip>

namespace graphics {
// ===================================================================
// Fractal and Plotting Primitives (STUBS)
// ===================================================================

Fractal::Fractal(int width, int height, FractalType type)
    : width_(width), height_(height), type_(type) {}
void Fractal::set_params(double param1, double param2) { param1_ = param1; param2_ = param2; }
void Fractal::set_max_iter(int max_iter) { max_iter_ = max_iter; }
void Fractal::set_bounds(double x_min, double x_max, double y_min, double y_max) {
    x_min_ = x_min; x_max_ = x_max; y_min_ = y_min; y_max_ = y_max;
}
Canvas Fractal::render() const {
    Canvas c(width_, height_);
    for (int py = 0; py < height_; ++py) {
        for (int px = 0; px < width_; ++px) {
            double x0 = x_min_ + (x_max_ - x_min_) * px / (width_ - 1);
            double y0 = y_min_ + (y_max_ - y_min_) * py / (height_ - 1);
            double x = x0, y = y0;
            double cx = (type_ == FractalType::Julia) ? param1_ : x0;
            double cy = (type_ == FractalType::Julia) ? param2_ : y0;
            int iter = 0;
            while (x*x + y*y <= 4.0 && iter < max_iter_) {
                double xt = x*x - y*y + cx;
                y = 2*x*y + cy;
                x = xt;
                ++iter;
            }
            int v = static_cast<int>(255.0 * iter / max_iter_);
            Color col = (iter == max_iter_) ? Colors::Black : Color{static_cast<uint8_t>(v), 0, static_cast<uint8_t>(255-v), 255};
            c.set_pixel(px, py, col);
        }
    }
    return c;
}

FunctionPlot::FunctionPlot(int width, int height)
    : width_(width), height_(height) {}
void FunctionPlot::set_equation(const std::string& expr) { expr_ = expr; }
void FunctionPlot::set_range(double x_min, double x_max) { x_min_ = x_min; x_max_ = x_max; }
void FunctionPlot::set_samples(int n) { samples_ = n; }
void FunctionPlot::set_color(Color c) { color_ = c; }
Canvas FunctionPlot::render() const {
    Canvas c(width_, height_);
    // Simple parser: only supports "sin(x)", "cos(x)", "x", "x^2", etc.
    auto eval = [](const std::string& expr, double x) -> double {
        if (expr == "x") return x;
        if (expr == "sin(x)") return std::sin(x);
        if (expr == "cos(x)") return std::cos(x);
        if (expr == "x^2") return x*x;
        if (expr == "exp(x)") return std::exp(x);
        if (expr == "log(x)") return std::log(x);
        return 0.0;
    };
    // Find y min/max for scaling
    double y_min = 1e9, y_max = -1e9;
    std::vector<double> xs(samples_), ys(samples_);
    for (int i = 0; i < samples_; ++i) {
        double x = x_min_ + (x_max_ - x_min_) * i / (samples_ - 1);
        double y = eval(expr_, x);
        xs[i] = x; ys[i] = y;
        y_min = std::min(y_min, y);
        y_max = std::max(y_max, y);
    }
    if (y_min == y_max) y_max = y_min + 1.0;
    // Draw axes
    int margin = 40;
    int plot_w = width_ - 2*margin, plot_h = height_ - 2*margin;
    c.draw_rect(margin, margin, plot_w, plot_h, Colors::Gray, false);
    // Draw function
    for (int i = 1; i < samples_; ++i) {
        int x0 = margin + static_cast<int>(plot_w * (xs[i-1] - x_min_) / (x_max_ - x_min_));
        int y0 = margin + plot_h - static_cast<int>(plot_h * (ys[i-1] - y_min) / (y_max - y_min));
        int x1 = margin + static_cast<int>(plot_w * (xs[i] - x_min_) / (x_max_ - x_min_));
        int y1 = margin + plot_h - static_cast<int>(plot_h * (ys[i] - y_min) / (y_max - y_min));
        c.draw_line(x0, y0, x1, y1, color_);
    }
    return c;
}

ParametricPlot::ParametricPlot(int width, int height)
    : width_(width), height_(height) {}
void ParametricPlot::set_equations(const std::string& x_expr, const std::string& y_expr) {
    x_expr_ = x_expr; y_expr_ = y_expr;
}
void ParametricPlot::set_t_range(double t_min, double t_max) { t_min_ = t_min; t_max_ = t_max; }
void ParametricPlot::set_samples(int n) { samples_ = n; }
void ParametricPlot::set_color(Color c) { color_ = c; }
Canvas ParametricPlot::render() const {
    Canvas c(width_, height_);
    // Simple parser: only supports "t", "sin(t)", "cos(t)", "t^2"
    auto eval = [](const std::string& expr, double t) -> double {
        if (expr == "t") return t;
        if (expr == "sin(t)") return std::sin(t);
        if (expr == "cos(t)") return std::cos(t);
        if (expr == "t^2") return t*t;
        return 0.0;
    };
    std::vector<double> xs(samples_), ys(samples_);
    double x_min = 1e9, x_max = -1e9, y_min = 1e9, y_max = -1e9;
    for (int i = 0; i < samples_; ++i) {
        double t = t_min_ + (t_max_ - t_min_) * i / (samples_ - 1);
        double x = eval(x_expr_, t);
        double y = eval(y_expr_, t);
        xs[i] = x; ys[i] = y;
        x_min = std::min(x_min, x); x_max = std::max(x_max, x);
        y_min = std::min(y_min, y); y_max = std::max(y_max, y);
    }
    if (x_min == x_max) x_max = x_min + 1.0;
    if (y_min == y_max) y_max = y_min + 1.0;
    int margin = 40;
    int plot_w = width_ - 2*margin, plot_h = height_ - 2*margin;
    c.draw_rect(margin, margin, plot_w, plot_h, Colors::Gray, false);
    for (int i = 1; i < samples_; ++i) {
        int x0 = margin + static_cast<int>(plot_w * (xs[i-1] - x_min) / (x_max - x_min));
        int y0 = margin + plot_h - static_cast<int>(plot_h * (ys[i-1] - y_min) / (y_max - y_min));
        int x1 = margin + static_cast<int>(plot_w * (xs[i] - x_min) / (x_max - x_min));
        int y1 = margin + plot_h - static_cast<int>(plot_h * (ys[i] - y_min) / (y_max - y_min));
        c.draw_line(x0, y0, x1, y1, color_);
    }
    return c;
}

PolarPlot::PolarPlot(int width, int height)
    : width_(width), height_(height) {}
void PolarPlot::set_equation(const std::string& expr) { expr_ = expr; }
void PolarPlot::set_theta_range(double theta_min, double theta_max) { theta_min_ = theta_min; theta_max_ = theta_max; }
void PolarPlot::set_samples(int n) { samples_ = n; }
void PolarPlot::set_color(Color c) { color_ = c; }
Canvas PolarPlot::render() const {
    Canvas c(width_, height_);
    // Simple parser: only supports "theta", "sin(theta)", "cos(theta)", "1+sin(5*theta)"
    auto eval = [](const std::string& expr, double theta) -> double {
        if (expr == "theta") return theta;
        if (expr == "sin(theta)") return std::sin(theta);
        if (expr == "cos(theta)") return std::cos(theta);
        if (expr == "1+sin(5*theta)") return 1.0 + std::sin(5*theta);
        return 0.0;
    };
    std::vector<double> rs(samples_);
    double r_min = 1e9, r_max = -1e9;
    for (int i = 0; i < samples_; ++i) {
        double theta = theta_min_ + (theta_max_ - theta_min_) * i / (samples_ - 1);
        double r = eval(expr_, theta);
        rs[i] = r;
        r_min = std::min(r_min, r);
        r_max = std::max(r_max, r);
    }
    if (r_min == r_max) r_max = r_min + 1.0;
    int cx = width_ / 2, cy = height_ / 2;
    double scale = 0.45 * std::min(width_, height_) / (r_max - r_min);
    for (int i = 1; i < samples_; ++i) {
        double t0 = theta_min_ + (theta_max_ - theta_min_) * (i-1) / (samples_ - 1);
        double t1 = theta_min_ + (theta_max_ - theta_min_) * i / (samples_ - 1);
        int x0 = cx + static_cast<int>(scale * rs[i-1] * std::cos(t0));
        int y0 = cy - static_cast<int>(scale * rs[i-1] * std::sin(t0));
        int x1 = cx + static_cast<int>(scale * rs[i] * std::cos(t1));
        int y1 = cy - static_cast<int>(scale * rs[i] * std::sin(t1));
        c.draw_line(x0, y0, x1, y1, color_);
    }
    return c;
}

HistogramPlot::HistogramPlot(int width, int height)
    : width_(width), height_(height) {}
void HistogramPlot::set_data(const std::vector<double>& values) { values_ = values; }
void HistogramPlot::set_bins(int n) { bins_ = n; }
void HistogramPlot::set_color(Color c) { color_ = c; }
Canvas HistogramPlot::render() const {
    Canvas c(width_, height_);
    if (values_.empty()) return c;
    double v_min = *std::min_element(values_.begin(), values_.end());
    double v_max = *std::max_element(values_.begin(), values_.end());
    if (v_min == v_max) v_max = v_min + 1.0;
    std::vector<int> bins(bins_, 0);
    for (double v : values_) {
        int idx = static_cast<int>((v - v_min) / (v_max - v_min) * bins_);
        if (idx < 0) idx = 0;
        if (idx >= bins_) idx = bins_ - 1;
        bins[idx]++;
    }
    int max_count = *std::max_element(bins.begin(), bins.end());
    int margin = 40;
    int plot_w = width_ - 2*margin, plot_h = height_ - 2*margin;
    c.draw_rect(margin, margin, plot_w, plot_h, Colors::Gray, false);
    for (int i = 0; i < bins_; ++i) {
        int x0 = margin + static_cast<int>(plot_w * i / static_cast<double>(bins_));
        int x1 = margin + static_cast<int>(plot_w * (i+1) / static_cast<double>(bins_));
        int y1 = margin + plot_h;
        int y0 = y1 - static_cast<int>(plot_h * bins[i] / static_cast<double>(max_count));
        c.draw_rect(x0, y0, x1-x0-1, y1-y0, color_, true);
        c.draw_rect(x0, y0, x1-x0-1, y1-y0, Colors::Black, false);
    }
    return c;
}

// ===================================================================
// Built-in 8×8 bitmap font (printable ASCII 32-126)
// Each glyph is 8 columns x 8 rows, stored as 8 bytes (1 bit per col).
// Bit 0 = leftmost column, bit 7 = rightmost (LSB-first row order).
//
// This is the well-known public-domain "font8x8_basic" bitmap font
// (https://github.com/dhepper/font8x8, itself derived from the classic
// public-domain IBM VGA font set). It replaces an earlier hand-rolled 5x7
// font that was too small/blocky to read comfortably in the app windows
// built on this Canvas (movie_editor, cad_builder, screen_builder, etc.) —
// 8x8 glyphs with real curves/serifs are noticeably more legible at the
// same on-screen scale.
// ===================================================================

static const uint8_t FONT_8x8[][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 '#'
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 36 '$'
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // 37 '%'
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // 38 '&'
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // 39 '''
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // 40 '('
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // 41 ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 '*'
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // 44 ','
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // 46 '.'
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // 47 '/'
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 48 '0'
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 49 '1'
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // 50 '2'
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // 51 '3'
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // 52 '4'
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // 53 '5'
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // 54 '6'
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 55 '7'
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 56 '8'
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // 57 '9'
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // 58 ':'
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // 59 ';'
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, // 60 '<'
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // 61 '='
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // 62 '>'
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, // 63 '?'
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, // 64 '@'
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // 65 'A'
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // 66 'B'
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // 67 'C'
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // 68 'D'
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, // 69 'E'
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, // 70 'F'
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, // 71 'G'
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // 72 'H'
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 73 'I'
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, // 74 'J'
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, // 75 'K'
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // 77 'M'
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // 78 'N'
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 79 'O'
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, // 80 'P'
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // 81 'Q'
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, // 82 'R'
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, // 83 'S'
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 84 'T'
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, // 85 'U'
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // 88 'X'
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, // 89 'Y'
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, // 90 'Z'
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // 91 '['
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 92 '\'
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // 93 ']'
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 95 '_'
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, // 97 'a'
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, // 98 'b'
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, // 99 'c'
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00}, // 100 'd'
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00}, // 101 'e'
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00}, // 102 'f'
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, // 103 'g'
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, // 104 'h'
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, // 105 'i'
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, // 106 'j'
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, // 107 'k'
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 108 'l'
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, // 109 'm'
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, // 110 'n'
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // 111 'o'
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, // 112 'p'
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, // 113 'q'
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, // 114 'r'
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, // 115 's'
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, // 116 't'
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, // 117 'u'
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 118 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00}, // 119 'w'
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, // 120 'x'
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, // 121 'y'
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, // 122 'z'
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, // 123 '{'
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 '|'
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, // 125 '}'
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 '~'
};
static constexpr int FONT_W = 8;
static constexpr int FONT_H = 8;
static constexpr int GLYPH_SPACING = 1; // pixels between characters

// ===================================================================
// Canvas
// ===================================================================

Canvas::Canvas(int width, int height, Color bg)
    : width_(width), height_(height)
{
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Canvas dimensions must be positive");
    pixels_.resize(static_cast<size_t>(width) * height * 4);
    fill(bg);
}

void Canvas::fill(Color c) {
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i * 4 + 0] = c.r;
        pixels_[i * 4 + 1] = c.g;
        pixels_[i * 4 + 2] = c.b;
        pixels_[i * 4 + 3] = c.a;
    }
}

void Canvas::set_pixel(int x, int y, Color c) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    blend_pixel(x, y, c);
}

Color Canvas::get_pixel(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        return {0, 0, 0, 0};
    int idx = (y * width_ + x) * 4;
    return {pixels_[idx], pixels_[idx+1], pixels_[idx+2], pixels_[idx+3]};
}

void Canvas::blend_pixel(int x, int y, Color c) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    int idx = (y * width_ + x) * 4;
    if (c.a == 255) {
        pixels_[idx+0] = c.r;
        pixels_[idx+1] = c.g;
        pixels_[idx+2] = c.b;
        pixels_[idx+3] = 255;
    } else {
        float a = c.a / 255.0f;
        pixels_[idx+0] = static_cast<uint8_t>(c.r * a + pixels_[idx+0] * (1.0f - a));
        pixels_[idx+1] = static_cast<uint8_t>(c.g * a + pixels_[idx+1] * (1.0f - a));
        pixels_[idx+2] = static_cast<uint8_t>(c.b * a + pixels_[idx+2] * (1.0f - a));
        pixels_[idx+3] = 255;
    }
}

// Bresenham's line with thickness
void Canvas::draw_line(int x0, int y0, int x1, int y1, Color c, int thickness) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int half = thickness / 2;

    while (true) {
        for (int ty = -half; ty <= half; ++ty)
            for (int tx = -half; tx <= half; ++tx)
                set_pixel(x0 + tx, y0 + ty, c);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Canvas::draw_rect(int x, int y, int w, int h, Color c, bool filled) {
    if (filled) {
        for (int py = y; py < y + h; ++py)
            for (int px = x; px < x + w; ++px)
                set_pixel(px, py, c);
    } else {
        draw_line(x, y, x + w - 1, y, c);
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, c);
        draw_line(x, y, x, y + h - 1, c);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, c);
    }
}

void Canvas::draw_circle(int cx, int cy, int radius, Color c, bool filled) {
    // Midpoint circle algorithm
    int x = radius, y = 0, err = 1 - radius;
    while (x >= y) {
        if (filled) {
            draw_line(cx - x, cy + y, cx + x, cy + y, c);
            draw_line(cx - x, cy - y, cx + x, cy - y, c);
            draw_line(cx - y, cy + x, cx + y, cy + x, c);
            draw_line(cx - y, cy - x, cx + y, cy - x, c);
        } else {
            set_pixel(cx + x, cy + y, c);
            set_pixel(cx - x, cy + y, c);
            set_pixel(cx + x, cy - y, c);
            set_pixel(cx - x, cy - y, c);
            set_pixel(cx + y, cy + x, c);
            set_pixel(cx - y, cy + x, c);
            set_pixel(cx + y, cy - x, c);
            set_pixel(cx - y, cy - x, c);
        }
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

void Canvas::draw_arc(int cx, int cy, int radius,
                      double start_angle_deg, double end_angle_deg,
                      Color c, int thickness) {
    if (radius <= 0 || thickness <= 0) return;

    auto norm_deg = [](double deg) {
        double out = std::fmod(deg, 360.0);
        if (out < 0.0) out += 360.0;
        return out;
    };

    double start = norm_deg(start_angle_deg);
    double end = norm_deg(end_angle_deg);
    if (end < start || (std::abs(end - start) < 1e-9 && std::abs(end_angle_deg - start_angle_deg) > 1e-9)) {
        end += 360.0;
    }

    const double span = end - start;
    const int segments = std::max(12, static_cast<int>(std::ceil((span / 360.0) * radius * 8.0)));
    const double delta = span / static_cast<double>(segments);

    int prev_x = 0;
    int prev_y = 0;
    bool has_prev = false;
    for (int i = 0; i <= segments; ++i) {
        const double deg = start + delta * static_cast<double>(i);
        const double rad = deg * 3.14159265358979323846 / 180.0;
        const int px = cx + static_cast<int>(std::lround(std::cos(rad) * radius));
        const int py = cy + static_cast<int>(std::lround(std::sin(rad) * radius));
        if (has_prev) {
            draw_line(prev_x, prev_y, px, py, c, thickness);
        }
        prev_x = px;
        prev_y = py;
        has_prev = true;
    }
}

void Canvas::draw_rounded_rect(int x, int y, int w, int h, int radius, Color c, bool filled) {
    if (w <= 0 || h <= 0) return;
    const int max_radius = std::max(0, std::min(w, h) / 2);
    radius = std::max(0, std::min(radius, max_radius));

    if (radius == 0) {
        draw_rect(x, y, w, h, c, filled);
        return;
    }

    if (filled) {
        draw_rect(x + radius, y, w - 2 * radius, h, c, true);
        draw_rect(x, y + radius, radius, h - 2 * radius, c, true);
        draw_rect(x + w - radius, y + radius, radius, h - 2 * radius, c, true);

        const int r2 = radius * radius;
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx * dx + dy * dy > r2) continue;
                set_pixel(x + radius + dx, y + radius + dy, c);
                set_pixel(x + w - radius - 1 + dx, y + radius + dy, c);
                set_pixel(x + radius + dx, y + h - radius - 1 + dy, c);
                set_pixel(x + w - radius - 1 + dx, y + h - radius - 1 + dy, c);
            }
        }
        return;
    }

    draw_line(x + radius, y, x + w - radius - 1, y, c);
    draw_line(x + radius, y + h - 1, x + w - radius - 1, y + h - 1, c);
    draw_line(x, y + radius, x, y + h - radius - 1, c);
    draw_line(x + w - 1, y + radius, x + w - 1, y + h - radius - 1, c);

    draw_arc(x + radius, y + radius, radius, 180.0, 270.0, c);
    draw_arc(x + w - radius - 1, y + radius, radius, 270.0, 360.0, c);
    draw_arc(x + w - radius - 1, y + h - radius - 1, radius, 0.0, 90.0, c);
    draw_arc(x + radius, y + h - radius - 1, radius, 90.0, 180.0, c);
}

// Scanline polygon fill + outline. Supports Solid, VerticalGradient and Hatch styles.
void Canvas::draw_polygon(const std::vector<std::pair<int,int>>& pts,
                          Color outline,
                          bool filled,
                          FillStyle style,
                          Color fill_color,
                          Color fill_color2,
                          int hatch_spacing) {
    if (pts.size() < 3) return;

    // Draw outline
    for (size_t i = 0; i < pts.size(); ++i) {
        auto [x0, y0] = pts[i];
        auto [x1, y1] = pts[(i+1) % pts.size()];
        draw_line(x0, y0, x1, y1, outline);
    }

    if (!filled) return;

    // Find Y bounds
    int ymin = pts[0].second, ymax = pts[0].second;
    int xmin = pts[0].first, xmax = pts[0].first;
    for (auto &p : pts) {
        ymin = std::min(ymin, p.second);
        ymax = std::max(ymax, p.second);
        xmin = std::min(xmin, p.first);
        xmax = std::max(xmax, p.first);
    }
    if (ymax < ymin) return;

    auto blend = [](const Color &a, const Color &b, double t) -> Color {
        uint8_t r = static_cast<uint8_t>(a.r + (b.r - a.r) * t);
        uint8_t g = static_cast<uint8_t>(a.g + (b.g - a.g) * t);
        uint8_t bl = static_cast<uint8_t>(a.b + (b.b - a.b) * t);
        uint8_t alpha = static_cast<uint8_t>(a.a + (b.a - a.a) * t);
        return {r, g, bl, alpha};
    };

    // For each scanline, compute intersections
    for (int y = ymin; y <= ymax; ++y) {
        std::vector<double> xs;
        for (size_t i = 0; i < pts.size(); ++i) {
            int x0 = pts[i].first, y0 = pts[i].second;
            int x1 = pts[(i+1) % pts.size()].first, y1 = pts[(i+1) % pts.size()].second;
            if (y0 == y1) continue; // horizontal edge
            int ymin_e = std::min(y0, y1);
            int ymax_e = std::max(y0, y1);
            if (y < ymin_e || y >= ymax_e) continue; // include top, exclude bottom
            double x = x0 + (double)(y - y0) * (double)(x1 - x0) / (double)(y1 - y0);
            xs.push_back(x);
        }
        if (xs.empty()) continue;
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2) {
            int x_start = static_cast<int>(std::ceil(xs[k]));
            int x_end = static_cast<int>(std::floor(xs[k+1]));
            if (x_end < x_start) continue;

            // choose fill color for this scanline
            Color line_fill = fill_color;
            if (style == FillStyle::VerticalGradient) {
                double t = (ymax == ymin) ? 0.0 : (double)(y - ymin) / (double)(ymax - ymin);
                line_fill = blend(fill_color, fill_color2, t);
            }

            for (int x = x_start; x <= x_end; ++x) {
                set_pixel(x, y, line_fill);
            }
        }
    }

    // Hatch overlay if requested
    if (style == FillStyle::Hatch && hatch_spacing > 0) {
        // 45-degree hatch lines across bounding box
        int diag = (xmax - xmin) + (ymax - ymin);
        for (int s = -diag; s <= diag; s += hatch_spacing) {
            int x0 = xmin + s;
            int y0 = ymin;
            int x1 = x0 + (ymax - ymin);
            int y1 = ymax;
            draw_line(x0, y0, x1, y1, outline);
        }
    }
}

void Canvas::draw_text(int x, int y, const std::string& text, Color c, int scale) {
    if (scale < 1) scale = 1;
    int cursor_x = x;
    for (char ch : text) {
        int idx = static_cast<int>(ch) - 32;
        if (idx < 0 || idx > 94) idx = 0; // unprintable → space
        const uint8_t* glyph = FONT_8x8[idx];
        for (int row = 0; row < FONT_H; ++row) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < FONT_W; ++col) {
                if (bits & (1 << col)) { // font8x8: bit 0 = leftmost column
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx)
                            set_pixel(cursor_x + col * scale + sx,
                                      y + row * scale + sy, c);
                }
            }
        }
        cursor_x += (FONT_W + GLYPH_SPACING) * scale;
    }
}

// ===================================================================
// BMP export (built-in, no dependencies)
// ===================================================================

bool Canvas::save_bmp(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    int row_bytes = width_ * 3;
    int padding = (4 - (row_bytes % 4)) % 4;
    int data_size = (row_bytes + padding) * height_;
    int file_size = 54 + data_size;

    // BMP header
    uint8_t header[54] = {};
    header[0] = 'B'; header[1] = 'M';
    std::memcpy(&header[2], &file_size, 4);
    int offset = 54;
    std::memcpy(&header[10], &offset, 4);
    int dib_size = 40;
    std::memcpy(&header[14], &dib_size, 4);
    std::memcpy(&header[18], &width_, 4);
    std::memcpy(&header[22], &height_, 4);
    uint16_t planes = 1;
    std::memcpy(&header[26], &planes, 2);
    uint16_t bpp = 24;
    std::memcpy(&header[28], &bpp, 2);
    std::memcpy(&header[34], &data_size, 4);

    f.write(reinterpret_cast<char*>(header), 54);

    // Pixel data (BMP is bottom-up, BGR)
    uint8_t pad[3] = {0, 0, 0};
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            int idx = (y * width_ + x) * 4;
            uint8_t bgr[3] = {pixels_[idx+2], pixels_[idx+1], pixels_[idx+0]};
            f.write(reinterpret_cast<char*>(bgr), 3);
        }
        if (padding > 0)
            f.write(reinterpret_cast<char*>(pad), padding);
    }
    return f.good();
}

// ===================================================================
// PNG / JPG export (via stb_image_write)
// ===================================================================

bool Canvas::save_png(const std::string& path) const {
    // stb expects top-down RGBA, which is what we have
    return stbi_write_png(path.c_str(), width_, height_, 4,
                          pixels_.data(), width_ * 4) != 0;
}

bool Canvas::save_jpg(const std::string& path, int quality) const {
    // stb expects top-down RGBA
    return stbi_write_jpg(path.c_str(), width_, height_, 4,
                          pixels_.data(), quality) != 0;
}

// ===================================================================
// Graph
// ===================================================================

Graph::Graph(int width, int height, GraphType type)
    : width_(width), height_(height), type_(type) {}

void Graph::set_title(const std::string& title) { title_ = title; }
void Graph::set_x_label(const std::string& label) { x_label_ = label; }
void Graph::set_y_label(const std::string& label) { y_label_ = label; }
void Graph::set_type(GraphType type) { type_ = type; }

void Graph::add_series(const DataSeries& series) {
    if (series.x_values.size() != series.y_values.size())
        throw std::invalid_argument("DataSeries: x and y must have same size");
    series_.push_back(series);
}

Graph::PlotArea Graph::compute_plot_area() const {
    int margin_left   = 70;
    int margin_right  = 20;
    int margin_top    = title_.empty() ? 20 : 40;
    int margin_bottom = x_label_.empty() ? 40 : 55;

    // Extra space for legend
    if (!series_.empty() && !series_[0].label.empty())
        margin_right = std::max(margin_right, 100);

    return {margin_left, margin_top,
            width_ - margin_left - margin_right,
            height_ - margin_top - margin_bottom};
}

static void compute_range(const std::vector<DataSeries>& series,
                          double& x_min, double& x_max,
                          double& y_min, double& y_max) {
    x_min = y_min =  std::numeric_limits<double>::max();
    x_max = y_max = -std::numeric_limits<double>::max();
    for (auto& s : series) {
        for (double v : s.x_values) { x_min = std::min(x_min, v); x_max = std::max(x_max, v); }
        for (double v : s.y_values) { y_min = std::min(y_min, v); y_max = std::max(y_max, v); }
    }
    // Pad y range by 5%
    double y_pad = (y_max - y_min) * 0.05;
    if (y_pad < 1e-9) y_pad = 1.0;
    y_min -= y_pad;
    y_max += y_pad;
    if (y_min > 0 && y_min - y_pad < 0) y_min = 0;
}

static std::string format_tick(double v) {
    std::ostringstream os;
    if (std::abs(v) < 1e-9) os << "0";
    else if (std::abs(v) >= 1000 || (std::abs(v) < 0.01 && v != 0))
        os << std::scientific << std::setprecision(1) << v;
    else
        os << std::fixed << std::setprecision(2) << v;
    return os.str();
}

void Graph::draw_axes(Canvas& canvas, const PlotArea& area,
                      double x_min, double x_max,
                      double y_min, double y_max) const {
    // Background
    canvas.draw_rect(area.x, area.y, area.w, area.h, Colors::LightGray, true);

    // Axes lines
    canvas.draw_line(area.x, area.y + area.h, area.x + area.w, area.y + area.h,
                     Colors::Black, 2);
    canvas.draw_line(area.x, area.y, area.x, area.y + area.h,
                     Colors::Black, 2);

    // Grid lines and tick labels
    int num_y_ticks = 5;
    for (int i = 0; i <= num_y_ticks; ++i) {
        double frac = static_cast<double>(i) / num_y_ticks;
        int py = area.y + area.h - static_cast<int>(frac * area.h);
        double val = y_min + frac * (y_max - y_min);

        // Grid line
        if (i > 0 && i < num_y_ticks)
            canvas.draw_line(area.x + 1, py, area.x + area.w, py, Colors::Gray);

        // Tick label
        std::string label = format_tick(val);
        int lx = area.x - static_cast<int>(label.size()) * 6 - 4;
        canvas.draw_text(std::max(2, lx), py - 3, label, Colors::Black);
    }

    int num_x_ticks = std::min(10, static_cast<int>(area.w / 60));
    if (num_x_ticks < 2) num_x_ticks = 2;
    for (int i = 0; i <= num_x_ticks; ++i) {
        double frac = static_cast<double>(i) / num_x_ticks;
        int px = area.x + static_cast<int>(frac * area.w);
        double val = x_min + frac * (x_max - x_min);

        // Grid line
        if (i > 0 && i < num_x_ticks)
            canvas.draw_line(px, area.y, px, area.y + area.h - 1, Colors::Gray);

        // Tick label
        std::string label = format_tick(val);
        int lx = px - static_cast<int>(label.size()) * 3;
        canvas.draw_text(lx, area.y + area.h + 5, label, Colors::Black);
    }

    // Title
    if (!title_.empty()) {
        int tx = (width_ - static_cast<int>(title_.size()) * 12) / 2;
        canvas.draw_text(std::max(2, tx), 5, title_, Colors::Black, 2);
    }

    // X label
    if (!x_label_.empty()) {
        int lx = area.x + (area.w - static_cast<int>(x_label_.size()) * 6) / 2;
        canvas.draw_text(lx, height_ - 15, x_label_, Colors::DarkGray);
    }

    // Y label (drawn vertically as horizontal text rotated conceptually)
    if (!y_label_.empty()) {
        int ly = area.y + (area.h - static_cast<int>(y_label_.size()) * 6) / 2;
        // Simple: draw each character stacked vertically
        for (size_t i = 0; i < y_label_.size(); ++i) {
            canvas.draw_text(2, ly + static_cast<int>(i) * 9,
                             std::string(1, y_label_[i]), Colors::DarkGray);
        }
    }
}

void Graph::draw_line_series(Canvas& canvas, const PlotArea& area,
                             const DataSeries& s,
                             double x_min, double x_max,
                             double y_min, double y_max) const {
    if (s.x_values.size() < 2) return;

    auto map_x = [&](double v) -> int {
        return area.x + static_cast<int>((v - x_min) / (x_max - x_min) * area.w);
    };
    auto map_y = [&](double v) -> int {
        return area.y + area.h - static_cast<int>((v - y_min) / (y_max - y_min) * area.h);
    };

    for (size_t i = 1; i < s.x_values.size(); ++i) {
        canvas.draw_line(map_x(s.x_values[i-1]), map_y(s.y_values[i-1]),
                         map_x(s.x_values[i]),   map_y(s.y_values[i]),
                         s.color, 2);
    }

    // Draw data points
    for (size_t i = 0; i < s.x_values.size(); ++i) {
        canvas.draw_circle(map_x(s.x_values[i]), map_y(s.y_values[i]),
                           3, s.color, true);
    }
}

void Graph::draw_bar_series(Canvas& canvas, const PlotArea& area,
                            int series_idx, int total_series,
                            const DataSeries& s,
                            double x_min, double x_max,
                            double y_min, double y_max) const {
    if (s.x_values.empty()) return;

    int n = static_cast<int>(s.x_values.size());
    double group_width = area.w / static_cast<double>(n);
    double bar_width = (group_width * 0.7) / total_series;

    auto map_y = [&](double v) -> int {
        return area.y + area.h - static_cast<int>((v - y_min) / (y_max - y_min) * area.h);
    };

    int base_y = map_y(std::max(0.0, y_min));

    for (int i = 0; i < n; ++i) {
        int group_x = area.x + static_cast<int>(i * group_width + group_width * 0.15);
        int bx = group_x + static_cast<int>(series_idx * bar_width);
        int by = map_y(s.y_values[i]);
        int bh = base_y - by;
        if (bh < 0) { by = base_y; bh = -bh; }
        canvas.draw_rect(bx, by, static_cast<int>(bar_width), bh, s.color, true);
        canvas.draw_rect(bx, by, static_cast<int>(bar_width), bh, Colors::Black);
    }
}

void Graph::draw_scatter_series(Canvas& canvas, const PlotArea& area,
                                const DataSeries& s,
                                double x_min, double x_max,
                                double y_min, double y_max) const {
    auto map_x = [&](double v) -> int {
        return area.x + static_cast<int>((v - x_min) / (x_max - x_min) * area.w);
    };
    auto map_y = [&](double v) -> int {
        return area.y + area.h - static_cast<int>((v - y_min) / (y_max - y_min) * area.h);
    };

    for (size_t i = 0; i < s.x_values.size(); ++i) {
        canvas.draw_circle(map_x(s.x_values[i]), map_y(s.y_values[i]),
                           4, s.color, true);
        canvas.draw_circle(map_x(s.x_values[i]), map_y(s.y_values[i]),
                           4, Colors::Black, false);
    }
}

void Graph::draw_legend(Canvas& canvas, const PlotArea& area) const {
    bool has_labels = false;
    for (auto& s : series_)
        if (!s.label.empty()) { has_labels = true; break; }
    if (!has_labels) return;

    int lx = area.x + area.w + 10;
    int ly = area.y + 5;
    for (size_t i = 0; i < series_.size(); ++i) {
        if (series_[i].label.empty()) continue;
        canvas.draw_rect(lx, ly + 1, 12, 7, series_[i].color, true);
        canvas.draw_text(lx + 16, ly, series_[i].label, Colors::Black);
        ly += 14;
    }
}

Canvas Graph::render() const {
    if (series_.empty())
        throw std::runtime_error("Graph::render: no data series added");

    Canvas canvas(width_, height_, Colors::White);

    double x_min, x_max, y_min, y_max;
    compute_range(series_, x_min, x_max, y_min, y_max);

    PlotArea area = compute_plot_area();
    draw_axes(canvas, area, x_min, x_max, y_min, y_max);

    for (size_t i = 0; i < series_.size(); ++i) {
        switch (type_) {
            case GraphType::Line:
                draw_line_series(canvas, area, series_[i],
                                 x_min, x_max, y_min, y_max);
                break;
            case GraphType::Bar:
                draw_bar_series(canvas, area,
                                static_cast<int>(i),
                                static_cast<int>(series_.size()),
                                series_[i],
                                x_min, x_max, y_min, y_max);
                break;
            case GraphType::Scatter:
                draw_scatter_series(canvas, area, series_[i],
                                    x_min, x_max, y_min, y_max);
                break;
        }
    }

    draw_legend(canvas, area);
    return canvas;
}

// ===================================================================
// Table
// ===================================================================

Table::Table() {}

void Table::set_headers(const std::vector<std::string>& headers) {
    headers_ = headers;
}

void Table::add_row(const std::vector<std::string>& row) {
    rows_.push_back(row);
}

void Table::set_cell_padding(int px)       { cell_padding_ = px; }
void Table::set_font_scale(int scale)      { font_scale_ = std::max(1, scale); }
void Table::set_header_color(Color c)      { header_bg_ = c; }
void Table::set_border_color(Color c)      { border_color_ = c; }
void Table::set_alternate_row_color(Color c) { alt_row_color_ = c; }

int Table::compute_col_width(int col) const {
    size_t max_len = 0;
    if (col < static_cast<int>(headers_.size()))
        max_len = headers_[col].size();
    for (auto& row : rows_) {
        if (col < static_cast<int>(row.size()))
            max_len = std::max(max_len, row[col].size());
    }
    return static_cast<int>(max_len) * (FONT_W + GLYPH_SPACING) * font_scale_
           + 2 * cell_padding_;
}

Canvas Table::render() const {
    int num_cols = static_cast<int>(headers_.size());
    for (auto& row : rows_)
        num_cols = std::max(num_cols, static_cast<int>(row.size()));
    if (num_cols == 0)
        throw std::runtime_error("Table::render: no columns");

    int num_rows = static_cast<int>(rows_.size()) + (headers_.empty() ? 0 : 1);
    int row_h = FONT_H * font_scale_ + 2 * cell_padding_;

    std::vector<int> col_widths(num_cols);
    int total_w = 1; // 1px left border
    for (int c = 0; c < num_cols; ++c) {
        col_widths[c] = std::max(compute_col_width(c), 40);
        total_w += col_widths[c] + 1; // +1 for right border
    }
    int total_h = 1 + num_rows * (row_h + 1);

    Canvas canvas(total_w, total_h, Colors::White);

    // Draw rows
    int y = 0;
    int row_idx = 0;

    auto draw_row = [&](const std::vector<std::string>& cells,
                        Color bg, Color fg) {
        // Background
        canvas.draw_rect(0, y, total_w, row_h + 1, bg, true);

        // Horizontal border at top
        canvas.draw_line(0, y, total_w - 1, y, border_color_);

        // Cell text & vertical borders
        int cx = 0;
        canvas.draw_line(cx, y, cx, y + row_h, border_color_);
        for (int c = 0; c < num_cols; ++c) {
            cx += 1; // past the border
            std::string text = (c < static_cast<int>(cells.size())) ? cells[c] : "";
            canvas.draw_text(cx + cell_padding_,
                             y + cell_padding_,
                             text, fg, font_scale_);
            cx += col_widths[c];
            canvas.draw_line(cx, y, cx, y + row_h, border_color_);
        }

        y += row_h;
        ++row_idx;
    };

    // Header row
    if (!headers_.empty())
        draw_row(headers_, header_bg_, header_fg_);

    // Data rows
    for (size_t r = 0; r < rows_.size(); ++r) {
        Color bg = (r % 2 == 0) ? Colors::White : alt_row_color_;
        draw_row(rows_[r], bg, Colors::Black);
    }

    // Bottom border
    canvas.draw_line(0, y, total_w - 1, y, border_color_);

    return canvas;
}

} // namespace graphics
