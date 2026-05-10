#include "../headers/video_filters.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))

int main() {
    using namespace trekker::video;
    using namespace trekker::video::filters;

    // Gaussian blur produces same size output
    {
        auto src = VideoFrame::blank(PixelFormat::RGB24, 16, 16);
        auto out = gaussian_blur(src, 1.0f, 3);
        EXPECT_EQ(out.width, std::size_t(16));
        EXPECT_EQ(out.height, std::size_t(16));
    }

    // Box blur on gray frame produces non-negative output
    {
        auto src = VideoFrame::blank(PixelFormat::GRAY8, 8, 8);
        for (auto& v : src.planes[0].data) v = 128;
        auto out = box_blur(src, 2);
        EXPECT_EQ(out.planes[0].data[0], std::uint8_t(128));
    }

    // Color grade: brightness shift
    {
        auto src = VideoFrame::blank(PixelFormat::RGB24, 1, 1);
        src.planes[0].data[0] = 100;
        src.planes[0].data[1] = 100;
        src.planes[0].data[2] = 100;
        ColorGradeParams p; p.brightness = 0.2f;
        auto out = color_grade(src, p);
        EXPECT_TRUE(out.planes[0].data[0] > 100);
    }

    // LUT identity is no-op
    {
        auto src = VideoFrame::blank(PixelFormat::RGB24, 1, 1);
        src.planes[0].data[0] = 42; src.planes[0].data[1] = 100; src.planes[0].data[2] = 200;
        auto lut = Lut1D::identity();
        auto out = apply_lut(src, lut);
        EXPECT_EQ(out.planes[0].data[0], std::uint8_t(42));
        EXPECT_EQ(out.planes[0].data[1], std::uint8_t(100));
        EXPECT_EQ(out.planes[0].data[2], std::uint8_t(200));
    }

    // Temporal denoise: first frame passes through unchanged
    {
        TemporalDenoise td;
        auto src = VideoFrame::blank(PixelFormat::GRAY8, 4, 4);
        src.planes[0].data[0] = 77;
        auto out = td.process(src);
        EXPECT_EQ(out.planes[0].data[0], std::uint8_t(77));
    }

    return failed > 0 ? 1 : 0;
}
