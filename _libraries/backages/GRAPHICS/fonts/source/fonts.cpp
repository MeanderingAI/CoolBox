#include "../headers/fonts.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <utility>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace graphics::fonts {

struct FontFace::Impl {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo info{};
    bool loaded = false;
};

namespace {

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (char ch : text) {
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
        } else if (ch != '\r') {
            current.push_back(ch);
        }
    }
    lines.push_back(current);
    return lines;
}

std::vector<int> decode_utf8(const std::string& text) {
    std::vector<int> codepoints;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        int codepoint = 0;
        std::size_t width = 1;

        if ((lead & 0x80u) == 0) {
            codepoint = lead;
        } else if ((lead & 0xE0u) == 0xC0u && i + 1 < text.size()) {
            codepoint = ((lead & 0x1Fu) << 6)
                      | (static_cast<unsigned char>(text[i + 1]) & 0x3Fu);
            width = 2;
        } else if ((lead & 0xF0u) == 0xE0u && i + 2 < text.size()) {
            codepoint = ((lead & 0x0Fu) << 12)
                      | ((static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6)
                      | (static_cast<unsigned char>(text[i + 2]) & 0x3Fu);
            width = 3;
        } else if ((lead & 0xF8u) == 0xF0u && i + 3 < text.size()) {
            codepoint = ((lead & 0x07u) << 18)
                      | ((static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 12)
                      | ((static_cast<unsigned char>(text[i + 2]) & 0x3Fu) << 6)
                      | (static_cast<unsigned char>(text[i + 3]) & 0x3Fu);
            width = 4;
        } else {
            codepoint = '?';
        }

        codepoints.push_back(codepoint);
        i += width;
    }
    return codepoints;
}

// compute_bounds and resolve_scale are implemented as TextRenderer static
// helpers (definitions are below) so they can access FontFace's private Impl via friendship.

Color alpha_blend(Color dst, Color src, unsigned char coverage) {
    const int alpha = static_cast<int>(src.a) * static_cast<int>(coverage) / 255;
    const int inv_alpha = 255 - alpha;

    Color out;
    out.r = static_cast<std::uint8_t>((src.r * alpha + dst.r * inv_alpha) / 255);
    out.g = static_cast<std::uint8_t>((src.g * alpha + dst.g * inv_alpha) / 255);
    out.b = static_cast<std::uint8_t>((src.b * alpha + dst.b * inv_alpha) / 255);
    out.a = static_cast<std::uint8_t>(std::min(255, alpha + (static_cast<int>(dst.a) * inv_alpha) / 255));
    return out;
}

} // namespace

// Definitions for TextRenderer helpers (in the graphics::fonts namespace)
float graphics::fonts::TextRenderer::resolve_scale(const FontFace& font, float pixel_height) {
    const auto* impl = font.impl();
    if (!impl) return 1.0f;
    return stbtt_ScaleForPixelHeight(&impl->info, std::max(1.0f, pixel_height));
}

graphics::fonts::TextBounds graphics::fonts::TextRenderer::compute_bounds(const FontFace& font,
                                                                          const std::string& text,
                                                                          const float pixel_height) {
    graphics::fonts::TextBounds bounds;
    const auto* impl = font.impl();
    if (!impl || !impl->loaded || text.empty()) return bounds;

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&impl->info, &ascent, &descent, &line_gap);

    const float scale = graphics::fonts::TextRenderer::resolve_scale(font, pixel_height);
    const int scaled_ascent = static_cast<int>(std::ceil(ascent * scale));
    const int scaled_descent = static_cast<int>(std::ceil(std::abs(descent * scale)));
    const int scaled_gap = static_cast<int>(std::ceil(line_gap * scale));
    const int line_height = std::max(1, scaled_ascent + scaled_descent + scaled_gap);

    bounds.ascent = scaled_ascent;
    bounds.descent = scaled_descent;
    bounds.line_gap = scaled_gap;

    const auto lines = split_lines(text);
    bounds.height = std::max(1, static_cast<int>(lines.size()) * line_height);

    for (const std::string& line : lines) {
        const auto codepoints = decode_utf8(line);
        int width = 0;
        for (std::size_t i = 0; i < codepoints.size(); ++i) {
            int advance = 0;
            int left_bearing = 0;
            stbtt_GetCodepointHMetrics(&impl->info, codepoints[i], &advance, &left_bearing);
            width += static_cast<int>(std::round(advance * scale));
            if (i + 1 < codepoints.size()) {
                width += static_cast<int>(std::round(
                    stbtt_GetCodepointKernAdvance(&impl->info, codepoints[i], codepoints[i + 1]) * scale));
            }
        }
        bounds.width = std::max(bounds.width, width);
    }

    return bounds;
}

FontFace::FontFace() : impl_(std::make_unique<Impl>()) {}
FontFace::~FontFace() = default;
FontFace::FontFace(FontFace&&) noexcept = default;
FontFace& FontFace::operator=(FontFace&&) noexcept = default;

const FontFace::Impl* FontFace::impl() const {
    return impl_.get();
}

FontFace::Impl* FontFace::impl() {
    return impl_.get();
}

bool FontFace::load_from_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        clear();
        return false;
    }

    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return load_from_memory(std::move(bytes));
}

bool FontFace::load_from_memory(std::vector<unsigned char> bytes) {
    clear();
    if (bytes.empty()) {
        return false;
    }

    impl_->bytes = std::move(bytes);
    impl_->loaded = stbtt_InitFont(&impl_->info, impl_->bytes.data(), 0) != 0;
    if (!impl_->loaded) {
        impl_->bytes.clear();
    }
    return impl_->loaded;
}

void FontFace::clear() {
    impl_->bytes.clear();
    impl_->info = stbtt_fontinfo{};
    impl_->loaded = false;
}

bool FontFace::is_loaded() const {
    return impl_->loaded;
}

TextBounds TextRenderer::measure_text(const FontFace& font,
                                      const std::string& text,
                                      const float pixel_height) {
    if (!font.impl() || !font.impl()->loaded || pixel_height <= 0.0f) {
        return {};
    }
    return TextRenderer::compute_bounds(font, text, pixel_height);
}

bool TextRenderer::draw_text(Canvas& canvas,
                             const FontFace& font,
                             const int x,
                             const int y,
                             const std::string& text,
                             const Color color,
                             const float pixel_height) {
    if (!font.impl() || !font.impl()->loaded || text.empty() || pixel_height <= 0.0f) {
        return false;
    }

    const auto lines = split_lines(text);
    const auto bounds = TextRenderer::compute_bounds(font, text, pixel_height);
    const float scale = TextRenderer::resolve_scale(font, pixel_height);
    const int line_height = std::max(1, bounds.ascent + bounds.descent + bounds.line_gap);

    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const int baseline_y = y + bounds.ascent + static_cast<int>(line_index) * line_height;
        int pen_x = x;
        const auto codepoints = decode_utf8(lines[line_index]);

        for (std::size_t i = 0; i < codepoints.size(); ++i) {
            const int codepoint = codepoints[i];
            int glyph_w = 0;
            int glyph_h = 0;
            int xoff = 0;
            int yoff = 0;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(
                &font.impl()->info, scale, scale, codepoint, &glyph_w, &glyph_h, &xoff, &yoff);

            if (bitmap != nullptr) {
                for (int row = 0; row < glyph_h; ++row) {
                    for (int col = 0; col < glyph_w; ++col) {
                        const unsigned char coverage = bitmap[row * glyph_w + col];
                        if (coverage == 0) {
                            continue;
                        }
                        const int px = pen_x + xoff + col;
                        const int py = baseline_y + yoff + row;
                        if (px < 0 || py < 0 || px >= canvas.width() || py >= canvas.height()) {
                            continue;
                        }
                        const Color dst = canvas.get_pixel(px, py);
                        canvas.set_pixel(px, py, alpha_blend(dst, color, coverage));
                    }
                }
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            int advance = 0;
            int left_bearing = 0;
            stbtt_GetCodepointHMetrics(&font.impl()->info, codepoint, &advance, &left_bearing);
            pen_x += static_cast<int>(std::round(advance * scale));
            if (i + 1 < codepoints.size()) {
                pen_x += static_cast<int>(std::round(
                    stbtt_GetCodepointKernAdvance(&font.impl()->info, codepoint, codepoints[i + 1]) * scale));
            }
        }
    }

    return true;
}

} // namespace graphics::fonts
