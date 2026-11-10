#include "components.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace graphics {
namespace components {
namespace {

double unit(double value) {
    return std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
}

unsigned char channel(double value) {
    return static_cast<unsigned char>(std::lround(unit(value) * 255.0));
}

Color from_hsv(double hue, double saturation, double value, unsigned char alpha = 255) {
    hue = std::isfinite(hue) ? std::fmod(hue, 360.0) : 0.0;
    if (hue < 0.0) hue += 360.0;
    saturation = unit(saturation);
    value = unit(value);
    const double chroma = value * saturation;
    const double secondary = chroma * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    const double minimum = value - chroma;
    double red = 0.0, green = 0.0, blue = 0.0;
    if (hue < 60.0) { red = chroma; green = secondary; }
    else if (hue < 120.0) { red = secondary; green = chroma; }
    else if (hue < 180.0) { green = chroma; blue = secondary; }
    else if (hue < 240.0) { green = secondary; blue = chroma; }
    else if (hue < 300.0) { red = secondary; blue = chroma; }
    else { red = chroma; blue = secondary; }
    return {channel(red + minimum), channel(green + minimum), channel(blue + minimum), alpha};
}

struct Layout {
    int side;
    int hue_x;
    int opacity_y;
    int swatch_y;
    int strip_width;

    Layout(int width, int height)
        : side(std::min(width - 48, height - 64)),
          hue_x(8 + side + 8), opacity_y(8 + side + 8),
          swatch_y(opacity_y + 24), strip_width(width - 16) {}
};

Color checker(int x, int y) {
    const unsigned char shade = ((x / 6 + y / 6) % 2 == 0) ? 220 : 150;
    return {shade, shade, shade, 255};
}

Color over_checker(Color color, int x, int y) {
    const auto background = checker(x, y);
    const auto mix = [&](unsigned char foreground) {
        return static_cast<unsigned char>((foreground * color.a + background.r * (255 - color.a) + 127) / 255);
    };
    return {mix(color.r), mix(color.g), mix(color.b), 255};
}

} // namespace

ColorPickerModel& ColorPickerModel::set_hsv(double hue, double saturation, double value) {
    const auto converted = from_hsv(hue, saturation, value, a);
    return set_rgb(converted.r, converted.g, converted.b);
}

ColorPickerModel::HSV ColorPickerModel::hsv() const {
    const double red = r / 255.0, green = g / 255.0, blue = b / 255.0;
    const double maximum = std::max({red, green, blue});
    const double minimum = std::min({red, green, blue});
    const double delta = maximum - minimum;
    double hue = 0.0;
    if (delta > 0.0) {
        if (maximum == red) hue = 60.0 * std::fmod((green - blue) / delta, 6.0);
        else if (maximum == green) hue = 60.0 * ((blue - red) / delta + 2.0);
        else hue = 60.0 * ((red - green) / delta + 4.0);
        if (hue < 0.0) hue += 360.0;
    }
    return {hue, maximum == 0.0 ? 0.0 : delta / maximum, maximum};
}

void ColorPicker::set_color(Color color) {
    model_.set_rgb(color.r, color.g, color.b).set_alpha(color.a);
    const auto hsv = model_.hsv();
    hue_ = hsv.h;
    saturation_ = hsv.s;
    value_ = hsv.v;
}

Color ColorPicker::color() const {
    return {model_.r, model_.g, model_.b, model_.a};
}

bool ColorPicker::pointer(int px, int py, int x, int y, int width, int height) {
    if (width < 220 || height < 180) return false;
    const Layout layout(width, height);
    const auto local_x = static_cast<std::int64_t>(px) - x;
    const auto local_y = static_cast<std::int64_t>(py) - y;
    if (local_x >= 8 && local_x < 8 + layout.side &&
        local_y >= 8 && local_y < 8 + layout.side) {
        saturation_ = static_cast<double>(local_x - 8) / (layout.side - 1);
        value_ = 1.0 - static_cast<double>(local_y - 8) / (layout.side - 1);
    } else if (local_x >= layout.hue_x && local_x < layout.hue_x + 20 &&
               local_y >= 8 && local_y < 8 + layout.side) {
        hue_ = 360.0 * static_cast<double>(local_y - 8) / (layout.side - 1);
    } else if (local_x >= 8 && local_x < 8 + layout.strip_width &&
               local_y >= layout.opacity_y && local_y < layout.opacity_y + 16) {
        model_.set_alpha(channel(static_cast<double>(local_x - 8) / (layout.strip_width - 1)));
        return true;
    } else {
        return false;
    }
    model_.set_hsv(hue_, saturation_, value_);
    return true;
}

void ColorPicker::draw(Canvas& canvas, int x, int y, int width, int height) const {
    if (width < 220 || height < 180) return;
    const Layout layout(width, height);
    const int saturation_mark = static_cast<int>(std::lround(saturation_ * (layout.side - 1)));
    const int value_mark = static_cast<int>(std::lround((1.0 - value_) * (layout.side - 1)));
    const int hue_mark = static_cast<int>(std::lround(hue_ / 360.0 * (layout.side - 1)));
    const int alpha_mark = static_cast<int>(std::lround(model_.a / 255.0 * (layout.strip_width - 1)));
    // Iterate only the visible canvas intersection; even oversized/offscreen bounds are cheap.
    const auto begin_x = std::max<std::int64_t>(0, x);
    const auto begin_y = std::max<std::int64_t>(0, y);
    const auto end_x = std::min<std::int64_t>(canvas.width(), static_cast<std::int64_t>(x) + width);
    const auto end_y = std::min<std::int64_t>(canvas.height(), static_cast<std::int64_t>(y) + height);
    for (auto cy = begin_y; cy < end_y; ++cy) {
        for (auto cx = begin_x; cx < end_x; ++cx) {
            const int lx = static_cast<int>(cx - x);
            const int ly = static_cast<int>(cy - y);
            Color pixel{38, 40, 46, 255};
            bool mark = false;
            if (lx >= 8 && lx < 8 + layout.side && ly >= 8 && ly < 8 + layout.side) {
                const int sx = lx - 8, sy = ly - 8;
                pixel = from_hsv(hue_, static_cast<double>(sx) / (layout.side - 1),
                                 1.0 - static_cast<double>(sy) / (layout.side - 1));
                const int dx = std::abs(sx - saturation_mark), dy = std::abs(sy - value_mark);
                mark = (dx == 3 && dy <= 3) || (dy == 3 && dx <= 3);
            } else if (lx >= layout.hue_x && lx < layout.hue_x + 20 &&
                       ly >= 8 && ly < 8 + layout.side) {
                pixel = from_hsv(360.0 * (ly - 8) / (layout.side - 1), 1.0, 1.0);
                mark = ly - 8 == hue_mark && (lx < layout.hue_x + 3 || lx >= layout.hue_x + 17);
            } else if (lx >= 8 && lx < 8 + layout.strip_width &&
                       ly >= layout.opacity_y && ly < layout.opacity_y + 16) {
                Color sample = color();
                sample.a = channel(static_cast<double>(lx - 8) / (layout.strip_width - 1));
                pixel = over_checker(sample, lx - 8, ly - layout.opacity_y);
                mark = lx - 8 == alpha_mark &&
                       (ly < layout.opacity_y + 3 || ly >= layout.opacity_y + 13);
            } else if (lx >= 8 && lx < 8 + layout.strip_width &&
                       ly >= layout.swatch_y && ly < layout.swatch_y + 16) {
                pixel = over_checker(color(), lx - 8, ly - layout.swatch_y);
            } else if (lx >= layout.hue_x + 28 && lx < width - 8 &&
                       ly >= 8 && ly < 8 + layout.side) {
                pixel = over_checker(color(), lx - layout.hue_x - 28, ly - 8);
            }
            if (mark) {
                pixel = (pixel.r * 299 + pixel.g * 587 + pixel.b * 114 > 128000)
                    ? Color{0, 0, 0, 255} : Color{255, 255, 255, 255};
            }
            canvas.set_pixel(static_cast<int>(cx), static_cast<int>(cy), pixel);
        }
    }
}

} // namespace components
} // namespace graphics
