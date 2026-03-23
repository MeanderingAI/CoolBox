#include <graphics/texture_loader.h>

#ifdef HAVE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace graphics {

bool loadTextureFromFile(const std::string& path, Texture &out) {
    int w=0,h=0,c=0;
    unsigned char *data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if(!data) return false;
    out.width = w;
    out.height = h;
    out.channels = c;
    size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
    out.pixels.assign(data, data + sz);
    stbi_image_free(data);
    return true;
}

} // namespace graphics

#else

#include <fstream>
#include <cstdint>

namespace graphics {

// Minimal BMP loader fallback
static bool loadBMP(const std::string &path, Texture &out) {
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs) return false;

    uint16_t bfType;
    ifs.read(reinterpret_cast<char*>(&bfType), sizeof(bfType));
    if(!ifs || bfType != 0x4D42) return false;

    uint32_t bfSize = 0;
    ifs.read(reinterpret_cast<char*>(&bfSize), sizeof(bfSize));
    uint32_t bfReserved = 0;
    ifs.read(reinterpret_cast<char*>(&bfReserved), sizeof(uint32_t));
    uint32_t bfOffBits = 0;
    ifs.read(reinterpret_cast<char*>(&bfOffBits), sizeof(bfOffBits));

    uint32_t biSize = 0;
    ifs.read(reinterpret_cast<char*>(&biSize), sizeof(biSize));
    if(biSize < 40) return false;

    int32_t biWidth = 0, biHeight = 0;
    uint16_t biPlanes = 0;
    uint16_t biBitCount = 0;
    uint32_t biCompression = 0;
    uint32_t biSizeImage = 0;

    ifs.read(reinterpret_cast<char*>(&biWidth), sizeof(biWidth));
    ifs.read(reinterpret_cast<char*>(&biHeight), sizeof(biHeight));
    ifs.read(reinterpret_cast<char*>(&biPlanes), sizeof(biPlanes));
    ifs.read(reinterpret_cast<char*>(&biBitCount), sizeof(biBitCount));
    ifs.read(reinterpret_cast<char*>(&biCompression), sizeof(biCompression));
    ifs.read(reinterpret_cast<char*>(&biSizeImage), sizeof(biSizeImage));

    if(biCompression != 0) return false;
    if(biBitCount != 24 && biBitCount != 32) return false;

    int width = biWidth;
    int height = std::abs(biHeight);
    int channels = (biBitCount == 24) ? 3 : 4;

    ifs.seekg(static_cast<std::streamoff>(bfOffBits), std::ios::beg);
    if(!ifs) return false;

    size_t rowSizeUnpadded = static_cast<size_t>(width) * channels;
    size_t rowPadding = 0;
    if(biBitCount == 24) {
        size_t rowSizePadded = ((width * 3 + 3) / 4) * 4;
        rowPadding = rowSizePadded - (width * 3);
    }

    out.width = width;
    out.height = height;
    out.channels = channels;
    out.pixels.clear();
    out.pixels.reserve(static_cast<size_t>(width) * static_cast<size_t>(height) * channels);

    bool topDown = (biHeight < 0);
    std::vector<unsigned char> rowBuf(static_cast<size_t>(width) * channels + rowPadding);
    std::vector<unsigned char> allPixels(static_cast<size_t>(width) * static_cast<size_t>(height) * channels);
    for(int y = 0; y < height; ++y) {
        ifs.read(reinterpret_cast<char*>(rowBuf.data()), static_cast<std::streamsize>(width * channels + rowPadding));
        if(!ifs) return false;
        int dstY = topDown ? y : (height - 1 - y);
        unsigned char *dst = allPixels.data() + static_cast<size_t>(dstY) * static_cast<size_t>(width) * channels;
        if(channels == 3) {
            for(int x = 0; x < width; ++x) {
                size_t srcIdx = static_cast<size_t>(x) * 3;
                dst[x*3 + 0] = rowBuf[srcIdx + 2];
                dst[x*3 + 1] = rowBuf[srcIdx + 1];
                dst[x*3 + 2] = rowBuf[srcIdx + 0];
            }
        } else {
            for(int x = 0; x < width; ++x) {
                size_t srcIdx = static_cast<size_t>(x) * 4;
                dst[x*4 + 0] = rowBuf[srcIdx + 2];
                dst[x*4 + 1] = rowBuf[srcIdx + 1];
                dst[x*4 + 2] = rowBuf[srcIdx + 0];
                dst[x*4 + 3] = rowBuf[srcIdx + 3];
            }
        }
    }

    out.pixels.swap(allPixels);
    return true;
}

bool loadTextureFromFile(const std::string& path, Texture &out) {
    if(loadBMP(path, out)) return true;
    return false;
}

} // namespace graphics

#endif
