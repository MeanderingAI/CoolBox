#include "fonts.hpp"

#include "tyst_framework.hpp"

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
