#pragma once

#include "graphics.h"

#include <memory>
#include <string>
#include <vector>

namespace graphics::fonts {

struct TextBounds {
    int width = 0;
    int height = 0;
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
};

class FontFace {
public:
    FontFace();
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;
    FontFace(FontFace&&) noexcept;
    FontFace& operator=(FontFace&&) noexcept;

    bool load_from_file(const std::string& path);
    bool load_from_memory(std::vector<unsigned char> bytes);
    void clear();
    bool is_loaded() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    const Impl* impl() const;
    Impl* impl();

    friend class TextRenderer;
};

class TextRenderer {
public:
    static TextBounds measure_text(const FontFace& font,
                                   const std::string& text,
                                   float pixel_height = 18.0f);

    static bool draw_text(Canvas& canvas,
                          const FontFace& font,
                          int x,
                          int y,
                          const std::string& text,
                          Color color,
                          float pixel_height = 18.0f);
};

} // namespace graphics::fonts
