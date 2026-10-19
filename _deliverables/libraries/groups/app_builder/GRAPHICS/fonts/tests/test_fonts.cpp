#include "fonts.hpp"

#include "tyst_framework.hpp"
#include <cstdlib>
#include <filesystem>

TEST(GraphicsFontsTest, MissingFontFailsGracefully) {
    graphics::fonts::FontFace font;
    EXPECT_FALSE(font.is_loaded());
    EXPECT_FALSE(font.load_from_file("definitely-missing-font.ttf"));

    graphics::Canvas canvas(64, 32);
    EXPECT_FALSE(graphics::fonts::TextRenderer::draw_text(
        canvas, font, 2, 2, "Patent", graphics::Colors::Black, 14.0f));
}

TEST(GraphicsFontsTest, UnloadedFontMeasuresAsZero) {
    graphics::fonts::FontFace font;
    const auto bounds = graphics::fonts::TextRenderer::measure_text(font, "Line Art", 16.0f);
    EXPECT_EQ(bounds.width, 0);
    EXPECT_EQ(bounds.height, 0);
}

TEST(GraphicsFontsTest, TrueTypeGlyphsHaveDistinctMetricsAndShapes) {
    const char* override_font = std::getenv("COOLBOX_UI_FONT");
    std::string path = override_font ? override_font : "";
#ifdef _WIN32
    if (path.empty()) {
        const char* windir = std::getenv("WINDIR");
        if (windir) path = (std::filesystem::path(windir) / "Fonts" / "segoeui.ttf").string();
    }
#elif defined(__APPLE__)
    if (path.empty()) path = "/System/Library/Fonts/Supplemental/Arial.ttf";
#else
    if (path.empty()) path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
    if (!std::filesystem::exists(path)) TYST_SKIP() << "No system TrueType font available";
    graphics::fonts::FontFace font;
    ASSERT_TRUE(font.load_from_file(path));
    const auto narrow = graphics::fonts::TextRenderer::measure_text(font, "iii", 24);
    const auto wide = graphics::fonts::TextRenderer::measure_text(font, "WWW", 24);
    EXPECT_GT(wide.width, narrow.width);
    graphics::Canvas canvas(64, 40, graphics::Colors::Black);
    ASSERT_TRUE(graphics::fonts::TextRenderer::draw_text(canvas, font, 0, 0, "O", graphics::Colors::White, 30));
    int lit = 0;
    int dark = 0;
    for (int y = 10; y < 20; ++y) {
        for (int x = 1; x < 16; ++x) {
            if (canvas.get_pixel(x, y).r > 0) ++lit;
            else ++dark;
        }
    }
    EXPECT_GT(lit, 0);
    EXPECT_GT(dark, 0);
}
