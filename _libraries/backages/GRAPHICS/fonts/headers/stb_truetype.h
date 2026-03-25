// Minimal in-repo replacement for stb_truetype.h used by the fonts library.
// This provides a very small subset of the API used in this project so
// the project does not depend on external FetchContent. It is intentionally
// minimal and should be extended to implement full TTF parsing and rasterization.

#pragma once

#include <cstddef>

struct stbtt_fontinfo {
    const unsigned char* data = nullptr;
    int data_size = 0;
    int unitsPerEm = 1000;
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    int numGlyphs = 0;
    // minimal bookkeeping for later extension
};

extern "C" {

int stbtt_InitFont(stbtt_fontinfo* info, const unsigned char* data, int offset);
float stbtt_ScaleForPixelHeight(const stbtt_fontinfo* info, float pixels);
void stbtt_GetFontVMetrics(const stbtt_fontinfo* info, int* ascent, int* descent, int* lineGap);

unsigned char* stbtt_GetCodepointBitmap(const stbtt_fontinfo* info,
                                        float scale_x,
                                        float scale_y,
                                        int codepoint,
                                        int* width, int* height,
                                        int* xoff, int* yoff);
void stbtt_FreeBitmap(unsigned char* bitmap, void* userdata);

void stbtt_GetCodepointHMetrics(const stbtt_fontinfo* info, int codepoint, int* advanceWidth, int* leftBearing);
int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo* info, int ch1, int ch2);

}
