#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_GRAPHICS_ATOM_BOND_DISPLAY_H
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_GRAPHICS_ATOM_BOND_DISPLAY_H

#include <string>
#include <vector>
#include <sstream>
#include <cmath>

#include <graphics/primitives.h>
#include <graphics/font_bitmap.h>
#include <chemistry/periodic_table.h>

namespace graphics {

// Produces a collection of drawing primitives (circles + lines + text)
// representing atoms and bonds. These primitives are renderer-agnostic
// and can be painted into a canvas implementation.

inline std::vector<Primitive> render_atom_bond_primitives(
    const std::vector<std::string>& atoms,
    const std::vector<std::pair<int,int>>& bonds,
    float start_x = 10.0f,
    float start_y = 50.0f,
    float spacing = 48.0f)
{
    std::vector<Primitive> out;
    size_t n = atoms.size();
    if (n == 0) return out;

    // simple linear layout: atom i at (start_x + i*spacing, start_y)
    std::vector<std::pair<float,float>> pos(n);
    for (size_t i = 0; i < n; ++i) {
        pos[i] = { start_x + static_cast<float>(i) * spacing, start_y };
    }

    // Draw bonds as lines between centers
    for (auto &b : bonds) {
        int a = b.first;
        int c = b.second;
        if (a < 0 || a >= static_cast<int>(n) || c < 0 || c >= static_cast<int>(n)) continue;
        Line L{ pos[a].first, pos[a].second, pos[c].first, pos[c].second, 2.0f, rgba(80,80,80) };
        out.emplace_back(L);
    }

    // Draw atoms as circles with text (symbol + valence)
    for (size_t i = 0; i < n; ++i) {
        auto [x,y] = pos[i];
        Circle C{ x, y, 16.0f, rgba(200,220,255) };
        out.emplace_back(C);

        auto elem = chemistry::element_by_symbol(atoms[i]);
        std::ostringstream label;
        label << elem.symbol;
        if (elem.valence_electrons > 0) label << " (v=" << elem.valence_electrons << ")";
        Text T{ x - 12.0f, y - 6.0f, label.str(), 12, rgba(10,10,10) };
        out.emplace_back(T);
    }

    return out;
}

// Render a simple textbox primitive (rect + text bitmap)
inline std::vector<Primitive> render_text_box(const std::string& text, float x, float y, float w, float h) {
    std::vector<Primitive> out;
    Rect R{ x, y, w, h, rgba(240,240,240), false };
    out.emplace_back(R);

    int bw, bh;
    auto bmp = text_to_bitmap(text, bw, bh);
    Bitmap B{ x+4, y+4, bw, bh, bmp };
    out.emplace_back(B);

    return out;
}

} // namespace graphics
#endif  // COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_COMPONENTS_HEADERS_GRAPHICS_ATOM_BOND_DISPLAY_H
