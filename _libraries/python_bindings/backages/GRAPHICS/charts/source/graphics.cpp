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
// Built-in 5×7 bitmap font (printable ASCII 32–126)
// Each glyph is 5 columns × 7 rows, stored as 7 bytes (1 bit per col).
// Bit 4 = leftmost column, bit 0 = rightmost.
// ===================================================================

static const uint8_t FONT_5x7[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // 33 '!'
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x0A,0x1F,0x0A,0x1F,0x0A,0x00,0x00}, // 35 '#'
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}, // 36 '$'
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // 37 '%'
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D}, // 38 '&'
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00}, // 39 '''
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // 40 '('
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // 41 ')'
    {0x00,0x0A,0x04,0x1F,0x04,0x0A,0x00}, // 42 '*'
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x04,0x04,0x08}, // 44 ','
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x04,0x00}, // 46 '.'
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10}, // 47 '/'
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 48 '0'
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 49 '1'
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 50 '2'
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 51 '3'
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 52 '4'
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 53 '5'
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 54 '6'
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 55 '7'
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 56 '8'
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 57 '9'
    {0x00,0x04,0x00,0x00,0x04,0x00,0x00}, // 58 ':'
    {0x00,0x04,0x00,0x00,0x04,0x04,0x08}, // 59 ';'
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // 60 '<'
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}, // 61 '='
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // 62 '>'
    {0x0E,0x11,0x01,0x02,0x04,0x00,0x04}, // 63 '?'
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E}, // 64 '@'
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // 65 'A'
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // 66 'B'
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // 67 'C'
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // 68 'D'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // 69 'E'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // 70 'F'
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}, // 71 'G'
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // 72 'H'
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // 73 'I'
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // 74 'J'
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // 75 'K'
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // 76 'L'
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // 77 'M'
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // 78 'N'
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // 79 'O'
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // 80 'P'
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // 81 'Q'
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // 82 'R'
    {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E}, // 83 'S'
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // 84 'T'
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // 85 'U'
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}, // 86 'V'
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // 87 'W'
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // 88 'X'
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // 89 'Y'
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // 90 'Z'
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}, // 91 '['
    {0x10,0x10,0x08,0x04,0x02,0x01,0x01}, // 92 '\'
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}, // 93 ']'
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F}, // 95 '_'
    {0x08,0x04,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // 97 'a'
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}, // 98 'b'
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E}, // 99 'c'
    {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F}, // 100 'd'
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}, // 101 'e'
    {0x06,0x08,0x1E,0x08,0x08,0x08,0x08}, // 102 'f'
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}, // 103 'g'
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x11}, // 104 'h'
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}, // 105 'i'
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C}, // 106 'j'
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12}, // 107 'k'
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}, // 108 'l'
    {0x00,0x00,0x1A,0x15,0x15,0x15,0x15}, // 109 'm'
    {0x00,0x00,0x1E,0x11,0x11,0x11,0x11}, // 110 'n'
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}, // 111 'o'
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}, // 112 'p'
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01}, // 113 'q'
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10}, // 114 'r'
    {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E}, // 115 's'
    {0x08,0x08,0x1E,0x08,0x08,0x09,0x06}, // 116 't'
    {0x00,0x00,0x11,0x11,0x11,0x11,0x0F}, // 117 'u'
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04}, // 118 'v'
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A}, // 119 'w'
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // 120 'x'
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}, // 121 'y'
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}, // 122 'z'
    {0x02,0x04,0x04,0x08,0x04,0x04,0x02}, // 123 '{'
    {0x04,0x04,0x04,0x04,0x04,0x04,0x04}, // 124 '|'
    {0x08,0x04,0x04,0x02,0x04,0x04,0x08}, // 125 '}'
    {0x00,0x00,0x08,0x15,0x02,0x00,0x00}, // 126 '~'
};

static constexpr int FONT_W = 5;
static constexpr int FONT_H = 7;
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

void Canvas::draw_text(int x, int y, const std::string& text, Color c, int scale) {
    if (scale < 1) scale = 1;
    int cursor_x = x;
    for (char ch : text) {
        int idx = static_cast<int>(ch) - 32;
        if (idx < 0 || idx > 94) idx = 0; // unprintable → space
        const uint8_t* glyph = FONT_5x7[idx];
        for (int row = 0; row < FONT_H; ++row) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < FONT_W; ++col) {
                if (bits & (1 << (FONT_W - 1 - col))) {
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
