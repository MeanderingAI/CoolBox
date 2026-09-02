// Minimal implementation of a tiny subset of stb_truetype API used by the
// project's `fonts` code. This is intentionally simple and should be
// extended to implement proper TTF parsing and rasterization later.

#include "../headers/stb_truetype.h"
#include <cstring>
#include <cmath>
#include <cstdlib>

static inline unsigned int read_u32_be(const unsigned char* p) {
    return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}
static inline unsigned short read_u16_be(const unsigned char* p) {
    return (p[0]<<8)|p[1];
}
static inline short read_s16_be(const unsigned char* p) {
    return (short)read_u16_be(p);
}

int stbtt_InitFont(stbtt_fontinfo* info, const unsigned char* data, int /*offset*/) {
    if (!info || !data) return 0;
    info->data = data;
    // Very small parser: locate 'head' and 'hhea' and 'maxp' tables via table directory
    const unsigned char* p = data;
    unsigned int sfnt = read_u32_be(p);
    unsigned short numTables = read_u16_be(p+4);
    const unsigned char* table = p + 12;
    const unsigned char* head_t = nullptr;
    const unsigned char* hhea_t = nullptr;
    const unsigned char* maxp_t = nullptr;
    for (int i=0;i<numTables;i++) {
        unsigned int tag = read_u32_be(table + i*16);
        unsigned int off = read_u32_be(table + i*16 + 8);
        // tags as big-endian integers
        if (tag == 0x68656164) { // 'head'
            head_t = data + off;
        } else if (tag == 0x68686561) { // 'hhea'
            hhea_t = data + off;
        } else if (tag == 0x6D617870) { // 'maxp'
            maxp_t = data + off;
        }
    }
    // Defaults
    info->unitsPerEm = 1000;
    info->ascent = 800;
    info->descent = -200;
    info->lineGap = 0;
    info->numGlyphs = 0;

    if (head_t) {
        // head: unitsPerEm at offset 18 (uint16)
        info->unitsPerEm = read_u16_be(head_t + 18);
    }
    if (hhea_t) {
        info->ascent = read_s16_be(hhea_t + 4);
        info->descent = read_s16_be(hhea_t + 6);
        info->lineGap = read_s16_be(hhea_t + 8);
    }
    if (maxp_t) {
        // maxp version 0.5 or 1.0; numGlyphs at offset 4 (uint16)
        info->numGlyphs = read_u16_be(maxp_t + 4);
    }
    return 1;
}

float stbtt_ScaleForPixelHeight(const stbtt_fontinfo* info, float pixels) {
    if (!info || info->unitsPerEm == 0) return 1.0f;
    return pixels / static_cast<float>(info->unitsPerEm);
}

void stbtt_GetFontVMetrics(const stbtt_fontinfo* info, int* ascent, int* descent, int* lineGap) {
    if (!info) return;
    if (ascent) *ascent = info->ascent;
    if (descent) *descent = info->descent;
    if (lineGap) *lineGap = info->lineGap;
}

// Very naive bitmap: rectangular filled area representing glyph coverage.
unsigned char* stbtt_GetCodepointBitmap(const stbtt_fontinfo* info,
                                        float scale_x,
                                        float scale_y,
                                        int /*codepoint*/,
                                        int* width, int* height,
                                        int* xoff, int* yoff) {
    if (!info || !width || !height) return nullptr;
    // use unitsPerEm metrics to estimate size
    int w = static_cast<int>(std::ceil(info->unitsPerEm * scale_x * 0.6f));
    int h = static_cast<int>(std::ceil((info->ascent - info->descent) * scale_y));
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    unsigned char* bmp = (unsigned char*)std::malloc((size_t)w * (size_t)h);
    if (!bmp) return nullptr;
    // simple gradient vertical coverage to make glyphs distinguishable
    for (int yy = 0; yy < h; ++yy) {
        unsigned char v = static_cast<unsigned char>(255.0f * (0.2f + 0.8f * (1.0f - (float)yy / (float)h)));
        for (int xx = 0; xx < w; ++xx) bmp[yy * w + xx] = v;
    }
    *width = w;
    *height = h;
    if (xoff) *xoff = 0;
    if (yoff) *yoff = -static_cast<int>(std::ceil(info->ascent * scale_y));
    return bmp;
}

void stbtt_FreeBitmap(unsigned char* bitmap, void* /*userdata*/) {
    std::free(bitmap);
}

void stbtt_GetCodepointHMetrics(const stbtt_fontinfo* info, int /*codepoint*/, int* advanceWidth, int* leftBearing) {
    if (advanceWidth) *advanceWidth = info ? info->unitsPerEm/2 : 500;
    if (leftBearing) *leftBearing = 0;
}

int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo* /*info*/, int /*ch1*/, int /*ch2*/) {
    return 0;
}
