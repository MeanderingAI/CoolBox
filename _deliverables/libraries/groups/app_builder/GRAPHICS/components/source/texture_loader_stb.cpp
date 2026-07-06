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

// Minimal BMP loader: supports uncompressed 24-bit and 32-bit BMP (Windows BITMAPINFO)
static bool loadBMP(const std::string &path, Texture &out) {
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs) return false;

    // Read BITMAPFILEHEADER (14 bytes)
    uint16_t bfType;
    ifs.read(reinterpret_cast<char*>(&bfType), sizeof(bfType));
    if(!ifs || bfType != 0x4D42) return false; // 'BM'

    uint32_t bfSize = 0;
    ifs.read(reinterpret_cast<char*>(&bfSize), sizeof(bfSize));
    uint32_t bfReserved = 0;
    ifs.read(reinterpret_cast<char*>(&bfReserved), sizeof(uint32_t));
    uint32_t bfOffBits = 0;
    ifs.read(reinterpret_cast<char*>(&bfOffBits), sizeof(bfOffBits));

    // Read BITMAPINFOHEADER (at least 40 bytes)
    uint32_t biSize = 0;
    ifs.read(reinterpret_cast<char*>(&biSize), sizeof(biSize));
    if(biSize < 40) return false; // we expect at least the standard header

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

    // Skip the rest of the header fields we don't care about
    if(biSize > 28) {
        std::vector<char> skip(biSize - 28);
        ifs.read(skip.data(), skip.size());
    }

    if(biCompression != 0) return false; // only support BI_RGB (no compression)
    if(biBitCount != 24 && biBitCount != 32) return false; // support 24/32-bit

    int width = biWidth;
    int height = std::abs(biHeight);
    int channels = (biBitCount == 24) ? 3 : 4;

    // Seek to pixel data
    ifs.seekg(static_cast<std::streamoff>(bfOffBits), std::ios::beg);
    if(!ifs) return false;

    // Rows are padded to 4-byte boundaries for 24-bit BMP
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

    // BMP stores rows bottom-up unless height negative
    bool topDown = (biHeight < 0);

    std::vector<unsigned char> rowBuf(static_cast<size_t>(width) * channels + rowPadding);

    // Read rows into a temporary buffer then reorder if necessary
    std::vector<unsigned char> allPixels(static_cast<size_t>(width) * static_cast<size_t>(height) * channels);
    for(int y = 0; y < height; ++y) {
        ifs.read(reinterpret_cast<char*>(rowBuf.data()), static_cast<std::streamsize>(width * channels + rowPadding));
        if(!ifs) return false;
        int dstY = topDown ? y : (height - 1 - y);
        unsigned char *dst = allPixels.data() + static_cast<size_t>(dstY) * static_cast<size_t>(width) * channels;
        // BMP stores BGR(A) order; convert to RGB(A)
        if(channels == 3) {
            for(int x = 0; x < width; ++x) {
                size_t srcIdx = static_cast<size_t>(x) * 3;
                dst[x*3 + 0] = rowBuf[srcIdx + 2];
                dst[x*3 + 1] = rowBuf[srcIdx + 1];
                dst[x*3 + 2] = rowBuf[srcIdx + 0];
            }
        } else { // 4 channels
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
    // Try BMP as a lightweight fallback format
    if(loadBMP(path, out)) return true;

    // Could add additional simple loaders (PPM, TGA) or a JSON/dataformat-based loader here.
    return false;
}

} // namespace graphics

#endif
