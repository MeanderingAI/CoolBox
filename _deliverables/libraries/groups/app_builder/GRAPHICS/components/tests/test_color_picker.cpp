#include "components.hpp"
#include "tyst_framework.hpp"

#include <climits>
#include <limits>

namespace {

void expect_color(graphics::Color actual, graphics::Color expected) {
    TYST_EXPECT_EQ(actual.r, expected.r);
    TYST_EXPECT_EQ(actual.g, expected.g);
    TYST_EXPECT_EQ(actual.b, expected.b);
    TYST_EXPECT_EQ(actual.a, expected.a);
}

} // namespace

TYST_TEST(GraphicsColorPickerTest, ModelRGBCompatibilityAndHSVPrimaries) {
    using graphics::components::ColorPickerModel;
    ColorPickerModel model(17, 31, 49);
    TYST_EXPECT_EQ(model.a, 255);
    model.set_alpha(72).set_rgb(4, 5, 6);
    TYST_EXPECT_EQ(model.a, 72);
    const graphics::Color primaries[] = {
        {255, 0, 0, 72}, {255, 255, 0, 72}, {0, 255, 0, 72},
        {0, 255, 255, 72}, {0, 0, 255, 72}, {255, 0, 255, 72}
    };
    for (int sector = 0; sector < 6; ++sector) {
        model.set_hsv(sector * 60.0, 1.0, 1.0);
        expect_color({model.r, model.g, model.b, model.a}, primaries[sector]);
        const auto hsv = model.hsv();
        TYST_EXPECT_EQ(hsv.h, sector * 60.0);
        TYST_EXPECT_EQ(hsv.s, 1.0);
        TYST_EXPECT_EQ(hsv.v, 1.0);
    }
}

TYST_TEST(GraphicsColorPickerTest, ModelClampsWrapsAndHandlesAchromaticColors) {
    graphics::components::ColorPickerModel model(0, 0, 0, 19);
    model.set_hsv(-120.0, 2.0, 3.0);
    expect_color({model.r, model.g, model.b, model.a}, {0, 0, 255, 19});
    model.set_hsv(480.0, 1.0, 1.0);
    expect_color({model.r, model.g, model.b, model.a}, {0, 255, 0, 19});
    model.set_hsv(120.0, -1.0, 1.0);
    expect_color({model.r, model.g, model.b, model.a}, {255, 255, 255, 19});
    TYST_EXPECT_EQ(model.hsv().s, 0.0);
    model.set_hsv(120.0, 1.0, -1.0);
    expect_color({model.r, model.g, model.b, model.a}, {0, 0, 0, 19});
    TYST_EXPECT_EQ(model.hsv().h, 0.0);
    TYST_EXPECT_EQ(model.hsv().s, 0.0);
    model.set_hsv(std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::quiet_NaN(), 1.0);
    expect_color({model.r, model.g, model.b, model.a}, {255, 255, 255, 19});
}

TYST_TEST(GraphicsColorPickerTest, PointerSelectsHSVAndPreservesHueThroughBlack) {
    graphics::components::ColorPicker picker;
    picker.set_color({255, 0, 0, 73});
    // At 220x185: square [8,129), hue [137,157), opacity y=137.
    TYST_EXPECT_TRUE(picker.pointer(145, 48, 0, 0, 220, 185));
    expect_color(picker.color(), {0, 255, 0, 73});
    TYST_EXPECT_TRUE(picker.pointer(145, 88, 0, 0, 220, 185));
    expect_color(picker.color(), {0, 0, 255, 73});
    TYST_EXPECT_TRUE(picker.pointer(128, 128, 0, 0, 220, 185));
    expect_color(picker.color(), {0, 0, 0, 73});
    TYST_EXPECT_TRUE(picker.pointer(145, 48, 0, 0, 220, 185));
    expect_color(picker.color(), {0, 0, 0, 73});
    TYST_EXPECT_TRUE(picker.pointer(128, 8, 0, 0, 220, 185));
    expect_color(picker.color(), {0, 255, 0, 73});
    TYST_EXPECT_TRUE(picker.pointer(8, 8, 0, 0, 220, 185));
    expect_color(picker.color(), {255, 255, 255, 73});
}

TYST_TEST(GraphicsColorPickerTest, PointerBoundsOffsetsAlphaAndSmallSize) {
    graphics::components::ColorPicker picker;
    picker.set_color({17, 31, 49, 63});
    // At 450x180: square side=116, alpha spans local x=[8,442), y=[132,148).
    TYST_EXPECT_TRUE(picker.pointer(28, 167, 20, 30, 450, 180));
    expect_color(picker.color(), {17, 31, 49, 0});
    TYST_EXPECT_TRUE(picker.pointer(461, 167, 20, 30, 450, 180));
    expect_color(picker.color(), {17, 31, 49, 255});
    TYST_EXPECT_FALSE(picker.pointer(462, 167, 20, 30, 450, 180));
    TYST_EXPECT_FALSE(picker.pointer(27, 167, 20, 30, 450, 180));
    TYST_EXPECT_FALSE(picker.pointer(28, 186, 20, 30, 450, 180));
    TYST_EXPECT_FALSE(picker.pointer(INT_MIN, INT_MAX, INT_MAX, INT_MIN, 450, 180));
    TYST_EXPECT_FALSE(picker.pointer(28, 38, 20, 30, 219, 180));
    TYST_EXPECT_FALSE(picker.pointer(28, 38, 20, 30, 220, 179));
    TYST_EXPECT_FALSE(picker.pointer(28, 38, 20, 30, -1, -1));
    expect_color(picker.color(), {17, 31, 49, 255});
}

TYST_TEST(GraphicsColorPickerTest, RenderHSVAlphaEndpointsAndCheckerboardSwatch) {
    graphics::components::ColorPicker picker;
    picker.set_color({255, 0, 0, 0});
    graphics::Canvas canvas(450, 180);
    picker.draw(canvas, 0, 0, 450, 180);
    // Avoid the deliberately visible selection markers.
    expect_color(canvas.get_pixel(8, 8), {255, 255, 255, 255});
    expect_color(canvas.get_pixel(123, 8), {255, 0, 0, 255});
    expect_color(canvas.get_pixel(123, 123), {0, 0, 0, 255});
    expect_color(canvas.get_pixel(8, 140), {150, 150, 150, 255});
    expect_color(canvas.get_pixel(441, 140), {255, 0, 0, 255});
    expect_color(canvas.get_pixel(8, 156), {220, 220, 220, 255});
    expect_color(canvas.get_pixel(14, 156), {150, 150, 150, 255});
    picker.set_color({255, 0, 0, 255});
    picker.draw(canvas, 0, 0, 450, 180);
    expect_color(canvas.get_pixel(8, 156), {255, 0, 0, 255});
    expect_color(canvas.get_pixel(14, 156), {255, 0, 0, 255});
    picker.set_color({255, 0, 0, 128});
    picker.draw(canvas, 0, 0, 450, 180);
    expect_color(canvas.get_pixel(8, 156), {238, 110, 110, 255});
}

TYST_TEST(GraphicsColorPickerTest, DrawingClipsAndSmallBoundsAreNoOp) {
    const graphics::Color untouched{3, 4, 5, 255};
    graphics::Canvas canvas(240, 200, untouched);
    const graphics::components::ColorPicker picker;
    picker.draw(canvas, 10, 10, 219, 180);
    picker.draw(canvas, 10, 10, 220, 179);
    picker.draw(canvas, 10, 10, 0, 0);
    expect_color(canvas.get_pixel(10, 10), untouched);
    picker.draw(canvas, 10, 10, 220, 180);
    expect_color(canvas.get_pixel(9, 10), untouched);
    expect_color(canvas.get_pixel(230, 10), untouched);
    expect_color(canvas.get_pixel(10, 190), untouched);
    picker.draw(canvas, INT_MAX, INT_MAX, 220, 180);
    picker.draw(canvas, -100, -100, 220, 180);
    expect_color(canvas.get_pixel(230, 10), untouched);
}
