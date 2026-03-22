#pragma once

#include <string>
#include <variant>
#include <vector>

namespace graphics {

struct Color { unsigned char r,g,b,a; };

struct Circle { float x,y; float radius; Color color; };
struct Line   { float x1,y1,x2,y2; float thickness; Color color; };
struct Rect   { float x,y,w,h; Color color; bool fill; };
struct Text   { float x,y; std::string text; int font_size; Color color; };
struct Bitmap { float x,y; int w,h; std::vector<unsigned char> pixels; /* monochrome 0/1 bytes */ };

using Primitive = std::variant<Circle, Line, Rect, Text, Bitmap>;

inline Color rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a=255) {
    return Color{r,g,b,a};
}

} // namespace graphics
