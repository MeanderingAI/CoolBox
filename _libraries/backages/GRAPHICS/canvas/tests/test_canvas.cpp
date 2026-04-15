#include "tyst_framework.hpp"
#include <graphics/primitives.h>
#include <graphics/texture_loader.h>

#include <fstream>
#include <cstdio>

TEST(CanvasPrimitives, Rgba) {
    auto c = graphics::rgba(1,2,3,4);
    EXPECT_EQ(c.r, 1);
    EXPECT_EQ(c.g, 2);
    EXPECT_EQ(c.b, 3);
    EXPECT_EQ(c.a, 4);
}

TEST(CanvasPrimitives, VariantHold) {
    graphics::Circle circ{0.0f, 0.0f, 1.5f, graphics::rgba(10,20,30)};
    graphics::Primitive p = circ;
    EXPECT_TRUE(std::holds_alternative<graphics::Circle>(p));
    auto &c = std::get<graphics::Circle>(p);
    EXPECT_FLOAT_EQ(c.radius, 1.5f);
}

TEST(CanvasTextureLoader, LoadBMPFallback) {
    const char *path = "canvas_test_tmp.bmp";
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out.good());

    // Prepare minimal 2x1 24-bit BMP (bottom-up row order)
    uint16_t bfType = 0x4D42; // 'BM'
    uint32_t bfOffBits = 54;
    uint32_t rowPadded = 8; // (2*3 + padding) -> 8
    uint32_t bfSize = bfOffBits + rowPadded;
    out.write(reinterpret_cast<char*>(&bfType), 2);
    out.write(reinterpret_cast<char*>(&bfSize), 4);
    uint32_t zero32 = 0; out.write(reinterpret_cast<char*>(&zero32), 4);
    out.write(reinterpret_cast<char*>(&bfOffBits), 4);

    uint32_t biSize = 40; out.write(reinterpret_cast<char*>(&biSize), 4);
    int32_t biWidth = 2; out.write(reinterpret_cast<char*>(&biWidth), 4);
    int32_t biHeight = 1; out.write(reinterpret_cast<char*>(&biHeight), 4);
    uint16_t biPlanes = 1; out.write(reinterpret_cast<char*>(&biPlanes), 2);
    uint16_t biBitCount = 24; out.write(reinterpret_cast<char*>(&biBitCount), 2);
    uint32_t biCompression = 0; out.write(reinterpret_cast<char*>(&biCompression), 4);
    uint32_t biSizeImage = 0; out.write(reinterpret_cast<char*>(&biSizeImage), 4);
    int32_t biXPelsPerMeter = 0; out.write(reinterpret_cast<char*>(&biXPelsPerMeter), 4);
    int32_t biYPelsPerMeter = 0; out.write(reinterpret_cast<char*>(&biYPelsPerMeter), 4);
    uint32_t biClrUsed = 0; out.write(reinterpret_cast<char*>(&biClrUsed), 4);
    uint32_t biClrImportant = 0; out.write(reinterpret_cast<char*>(&biClrImportant), 4);

    // Pixel data: two pixels BGR. First pixel blue (0,0,255) -> BGR {255,0,0}
    unsigned char px1[3] = {255,0,0};
    // second pixel red (255,0,0) -> BGR {0,0,255}
    unsigned char px2[3] = {0,0,255};
    out.write(reinterpret_cast<char*>(px1), 3);
    out.write(reinterpret_cast<char*>(px2), 3);
    unsigned char pad[2] = {0,0}; out.write(reinterpret_cast<char*>(pad), 2);
    out.close();

    graphics::Texture t;
    bool ok = graphics::loadTextureFromFile(path, t);
    EXPECT_TRUE(ok);
    EXPECT_EQ(t.width, 2);
    EXPECT_EQ(t.height, 1);
    EXPECT_EQ(t.channels, 3);
    EXPECT_EQ(static_cast<int>(t.pixels.size()), 2*1*3);

    std::remove(path);
}

int main(int argc, char** argv) {
    tyst::framework::init(&argc, argv);
    return tyst::framework::run_all_tests();
}
