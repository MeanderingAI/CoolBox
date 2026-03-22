/* Minimal replacement for stb_image.h
 * Provides `stbi_load` (supports uncompressed 24/32-bit BMP only)
 * and `stbi_image_free`. This is intentionally small — it is NOT a
 * full stb_image implementation. Use the real stb_image.h for full format
 * support.
 *
 * License: public domain / MIT-compatible (short permissive notice)
 */
#ifndef MINIMAL_STB_IMAGE_H
#define MINIMAL_STB_IMAGE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char stbi_uc;

// load image from filename. supports only BMP (24/32-bit, BI_RGB)
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
void stbi_image_free(void *retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

// Implementation when STB_IMAGE_IMPLEMENTATION is defined
#ifdef STB_IMAGE_IMPLEMENTATION

#include <stdint.h>

static int stbi__read16le(FILE *f, uint16_t *out) {
    unsigned char b[2];
    if (fread(b,1,2,f) != 2) return 0;
    *out = (uint16_t)(b[0] | (b[1]<<8));
    return 1;
}

static int stbi__read32le(FILE *f, uint32_t *out) {
    unsigned char b[4];
    if (fread(b,1,4,f) != 4) return 0;
    *out = (uint32_t)(b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24));
    return 1;
}

stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) {
    if (!filename) return NULL;
    FILE *ifs = fopen(filename, "rb");
    if (!ifs) return NULL;

    // BITMAPFILEHEADER
    uint16_t bfType;
    if (!stbi__read16le(ifs, &bfType)) { fclose(ifs); return NULL; }
    if (bfType != 0x4D42) { fclose(ifs); return NULL; } // 'BM'

    uint32_t bfSize; if (!stbi__read32le(ifs, &bfSize)) { fclose(ifs); return NULL; }
    uint32_t bfReserved; if (!stbi__read32le(ifs, &bfReserved)) { fclose(ifs); return NULL; }
    uint32_t bfOffBits; if (!stbi__read32le(ifs, &bfOffBits)) { fclose(ifs); return NULL; }

    uint32_t biSize; if (!stbi__read32le(ifs, &biSize)) { fclose(ifs); return NULL; }
    if (biSize < 40) { fclose(ifs); return NULL; }

    int32_t biWidth=0, biHeight=0;
    if (!stbi__read32le(ifs, (uint32_t*)&biWidth)) { fclose(ifs); return NULL; }
    if (!stbi__read32le(ifs, (uint32_t*)&biHeight)) { fclose(ifs); return NULL; }
    uint16_t biPlanes; if (!stbi__read16le(ifs, &biPlanes)) { fclose(ifs); return NULL; }
    uint16_t biBitCount; if (!stbi__read16le(ifs, &biBitCount)) { fclose(ifs); return NULL; }
    uint32_t biCompression; if (!stbi__read32le(ifs, &biCompression)) { fclose(ifs); return NULL; }
    uint32_t biSizeImage; if (!stbi__read32le(ifs, &biSizeImage)) { fclose(ifs); return NULL; }

    // Seek past remaining header bytes
    if (biSize > 28) {
        long toskip = (long)(biSize - 28);
        if (fseek(ifs, toskip, SEEK_CUR) != 0) { fclose(ifs); return NULL; }
    }

    if (biCompression != 0) { fclose(ifs); return NULL; } // only BI_RGB
    if (biBitCount != 24 && biBitCount != 32) { fclose(ifs); return NULL; }

    int width = biWidth;
    int height = biHeight < 0 ? -biHeight : biHeight;
    int channels = (biBitCount == 24) ? 3 : 4;

    if (x) *x = width;
    if (y) *y = height;
    if (channels_in_file) *channels_in_file = channels;

    // allocate buffer (row-major top-left) in RGB(A) order
    size_t imgSize = (size_t)width * (size_t)height * (size_t)channels;
    stbi_uc *data = (stbi_uc*) malloc(imgSize);
    if (!data) { fclose(ifs); return NULL; }

    // move to pixel data
    if (fseek(ifs, (long)bfOffBits, SEEK_SET) != 0) { free(data); fclose(ifs); return NULL; }

    // BMP rows are padded to 4-byte boundaries for 24-bit
    size_t rowBytes = (size_t)width * channels;
    size_t rowPad = 0;
    if (biBitCount == 24) {
        size_t rowPadded = ((width*3 + 3) / 4) * 4;
        rowPad = rowPadded - (width*3);
    }

    // read rows bottom-up unless height negative (top-down)
    int topDown = (biHeight < 0);
    for (int row = 0; row < height; ++row) {
        int dstRow = topDown ? row : (height - 1 - row);
        stbi_uc *dst = data + (size_t)dstRow * (size_t)width * channels;
        // temporary buffer for a row (including padding)
        size_t tmpRowSize = (size_t)width * channels + rowPad;
        stbi_uc *tmp = (stbi_uc*) malloc(tmpRowSize);
        if (!tmp) { free(data); fclose(ifs); return NULL; }
        if (fread(tmp, 1, tmpRowSize, ifs) != tmpRowSize) { free(tmp); free(data); fclose(ifs); return NULL; }
        // BMP stored as B G R (A)
        if (channels == 3) {
            for (int xpix=0; xpix < width; ++xpix) {
                size_t s = (size_t)xpix * 3;
                dst[xpix*3 + 0] = tmp[s + 2];
                dst[xpix*3 + 1] = tmp[s + 1];
                dst[xpix*3 + 2] = tmp[s + 0];
            }
        } else {
            for (int xpix=0; xpix < width; ++xpix) {
                size_t s = (size_t)xpix * 4;
                dst[xpix*4 + 0] = tmp[s + 2];
                dst[xpix*4 + 1] = tmp[s + 1];
                dst[xpix*4 + 2] = tmp[s + 0];
                dst[xpix*4 + 3] = tmp[s + 3];
            }
        }
        free(tmp);
    }

    fclose(ifs);

    // If caller requested a specific number of channels, do a simple conversion
    if (desired_channels && desired_channels != channels) {
        int req = desired_channels;
        size_t reqSize = (size_t)width * (size_t)height * (size_t)req;
        stbi_uc *reqBuf = (stbi_uc*) malloc(reqSize);
        if (!reqBuf) { free(data); return NULL; }
        // only handle common conversions
        for (int i=0;i<width*height;++i) {
            stbi_uc r = data[i*channels + 0];
            stbi_uc g = data[i*channels + 1];
            stbi_uc b = data[i*channels + 2];
            stbi_uc a = (channels==4) ? data[i*channels + 3] : 255;
            if (req == 1) reqBuf[i] = (stbi_uc)((int)r*77 + (int)g*150 + (int)b*29 >> 8);
            else if (req == 2) { reqBuf[i*2+0] = (stbi_uc)((int)r*77 + (int)g*150 + (int)b*29 >> 8); reqBuf[i*2+1] = a; }
            else if (req == 3) { reqBuf[i*3+0]=r; reqBuf[i*3+1]=g; reqBuf[i*3+2]=b; }
            else if (req == 4) { reqBuf[i*4+0]=r; reqBuf[i*4+1]=g; reqBuf[i*4+2]=b; reqBuf[i*4+3]=a; }
        }
        free(data);
        data = reqBuf;
        if (channels_in_file) *channels_in_file = req;
    }

    return data;
}

void stbi_image_free(void *retval_from_stbi_load) {
    free(retval_from_stbi_load);
}

#endif // STB_IMAGE_IMPLEMENTATION

#endif // MINIMAL_STB_IMAGE_H
