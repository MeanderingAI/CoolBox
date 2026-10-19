#include "tyst_framework.hpp"
#include "plotting.hpp"

#include <limits>
#include <stdexcept>

TEST(PlottingTest, Renders2DWithConvenientDefaults) {
    const graphics::Canvas canvas = graphics::plot_2d(
        {0.0, 1.0, 2.0},
        {1.0, 3.0, 2.0});

    EXPECT_EQ(canvas.width(), 800);
    EXPECT_EQ(canvas.height(), 600);
}

TEST(PlottingTest, RejectsInvalid2DData) {
    EXPECT_THROW(graphics::plot_2d({1.0, 2.0}, {3.0}), std::invalid_argument);
    EXPECT_THROW(
        graphics::plot_2d({1.0}, {std::numeric_limits<double>::infinity()}),
        std::invalid_argument);
}

TEST(PlottingTest, Renders3DWithConvenientDefaults) {
    graphics::Plot3DOptions options;
    options.width = 320;
    options.height = 240;
    const graphics::Canvas canvas = graphics::plot_3d(
        {0.0, 1.0, 2.0},
        {1.0, 2.0, 1.0},
        {0.0, 1.5, 2.0},
        options);

    EXPECT_EQ(canvas.width(), 320);
    EXPECT_EQ(canvas.height(), 240);
}

TEST(PlottingTest, RejectsInvalid3DDataAndView) {
    EXPECT_THROW(
        graphics::plot_3d({0.0, 1.0}, {0.0}, {0.0, 1.0}),
        std::invalid_argument);

    graphics::Plot3DOptions options;
    options.elevation_degrees = 91.0;
    EXPECT_THROW(
        graphics::plot_3d({0.0}, {0.0}, {0.0}, options),
        std::invalid_argument);
}
