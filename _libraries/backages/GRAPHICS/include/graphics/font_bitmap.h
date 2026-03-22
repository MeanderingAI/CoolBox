#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace graphics {

// Lightweight placeholder ASCII bitmap generator.
// Returns an 8x8 monochrome bitmap (8 bytes, each bit is a pixel).
// This implementation uses a simple deterministic pattern as a
// placeholder font. Replace entries with a true font bitmap table
// if you need authentic glyph shapes.
inline std::array<uint8_t,8> char_to_8x8_bitmap(char c) {
    std::array<uint8_t,8> out{};
    uint8_t v = static_cast<uint8_t>(c);
    for (int r = 0; r < 8; ++r) {
        // spread character code bits across rows for a reproducible placeholder
        out[r] = static_cast<uint8_t>(((v << (r%5)) | (v >> (r%3))) & 0xFF);
    }
    return out;
}

// Convert text to a horizontal bitmap: each character occupies 8x8
inline std::vector<unsigned char> text_to_bitmap(const std::string& s, int& out_w, int& out_h) {
    out_h = 8;
    out_w = static_cast<int>(s.size()) * 8;
    std::vector<unsigned char> pixels(out_w * out_h);
    for (size_t i = 0; i < s.size(); ++i) {
        auto glyph = char_to_8x8_bitmap(s[i]);
        for (int r = 0; r < 8; ++r) {
            uint8_t row = glyph[r];
            for (int c = 0; c < 8; ++c) {
                bool bit = (row >> (7-c)) & 1;
                pixels[r * out_w + static_cast<int>(i)*8 + c] = bit ? 255 : 0;
            }
        }
    }
    return pixels;
}

} // namespace graphics
