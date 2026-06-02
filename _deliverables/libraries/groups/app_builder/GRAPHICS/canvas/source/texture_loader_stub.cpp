#include <graphics/texture_loader.h>
#include <iostream>

namespace graphics {

bool loadTextureFromFile(const std::string& path, Texture &out) {
#if defined(__APPLE__) && defined(__aarch64__)
    std::cerr << "[graphics::loadTextureFromFile] Not implemented for Apple Silicon (arm64). Requested: '" << path << "'\n";
    return false;
#else
    std::cerr << "[graphics::loadTextureFromFile] Platform stub called. Requested: '" << path << "'\n";
    return false;
#endif
}

} // namespace graphics
