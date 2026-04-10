// Canvas texture loader public header
#ifndef COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_CANVAS_HEADERS_GRAPHICS_TEXTURE_LOADER_H
#define COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_CANVAS_HEADERS_GRAPHICS_TEXTURE_LOADER_H

#include <string>
#include <vector>

namespace graphics {

struct Texture {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
};

// Load an image file into a CPU-side texture buffer.
// Returns true on success and fills the `out` texture.
bool loadTextureFromFile(const std::string& path, Texture &out);

} // namespace graphics
#endif  // COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_CANVAS_HEADERS_GRAPHICS_TEXTURE_LOADER_H
