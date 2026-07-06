#include "../headers/video_codec.h"
#include <cassert>
#include <cstring>

// Basic tests for video_codec — pixel format conversion and frame allocation.

static int passed = 0, failed = 0;

#define EXPECT_EQ(a, b) \
    do { if ((a) == (b)) { ++passed; } \
    else { ++failed; } } while(0)

#define EXPECT_TRUE(x) EXPECT_EQ(!!(x), true)

int main() {
    using namespace trekker::video;

    // Blank frame allocation
    {
        auto f = VideoFrame::blank(PixelFormat::YUV420P, 64, 48);
        EXPECT_EQ(f.width,  std::size_t(64));
        EXPECT_EQ(f.height, std::size_t(48));
        EXPECT_EQ(f.plane_count(), 3);
        EXPECT_EQ(f.planes[1].width,  std::size_t(32));
        EXPECT_EQ(f.planes[1].height, std::size_t(24));
    }

    // Round-trip: RGB → YUV444P → RGB
    {
        auto src = VideoFrame::blank(PixelFormat::RGB24, 2, 2);
        // Fill with red
        for (std::size_t i = 0; i < src.planes[0].data.size(); i += 3) {
            src.planes[0].data[i + 0] = 255;
            src.planes[0].data[i + 1] = 0;
            src.planes[0].data[i + 2] = 0;
        }
        auto yuv = convert_format(src, PixelFormat::YUV444P);
        auto back = convert_format(yuv, PixelFormat::RGB24);
        // Allow ±3 rounding error
        const int r = back.planes[0].data[0];
        EXPECT_TRUE(r >= 252 && r <= 255);
    }

    // GRAY8 conversion preserves luma
    {
        auto src = VideoFrame::blank(PixelFormat::RGB24, 1, 1);
        src.planes[0].data[0] = 100;
        src.planes[0].data[1] = 100;
        src.planes[0].data[2] = 100;
        auto gray = convert_format(src, PixelFormat::GRAY8);
        EXPECT_TRUE(gray.planes[0].data[0] >= 98 && gray.planes[0].data[0] <= 102);
    }

    return failed > 0 ? 1 : 0;
}
