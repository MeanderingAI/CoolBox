#include "tyst_framework.hpp"
#include "graphics.h"

#include <cstdio>
#include <fstream>
#include <cmath>

using namespace graphics;

// ===================================================================
// Canvas – basic operations
// ===================================================================

TEST(CanvasTest, ConstructorSetsSize) {
    Canvas c(100, 50);
    EXPECT_EQ(c.width(), 100);
    EXPECT_EQ(c.height(), 50);
}

TEST(CanvasTest, InvalidDimensionsThrow) {
    EXPECT_THROW(Canvas(0, 10), std::invalid_argument);
    EXPECT_THROW(Canvas(10, -1), std::invalid_argument);
}

TEST(CanvasTest, FillAndGetPixel) {
    Canvas c(10, 10, Colors::Red);
    Color p = c.get_pixel(5, 5);
    EXPECT_EQ(p.r, 220);
    EXPECT_EQ(p.g, 50);
    EXPECT_EQ(p.b, 50);
    EXPECT_EQ(p.a, 255);
}

TEST(CanvasTest, SetPixelAndRead) {
    Canvas c(10, 10);
    c.set_pixel(3, 4, Colors::Blue);
    Color p = c.get_pixel(3, 4);
    EXPECT_EQ(p, Colors::Blue);
}

TEST(CanvasTest, OutOfBoundsPixelIsSafe) {
    Canvas c(10, 10);
    // Should not crash
    c.set_pixel(-1, 0, Colors::Red);
    c.set_pixel(0, -1, Colors::Red);
    c.set_pixel(100, 0, Colors::Red);
    c.set_pixel(0, 100, Colors::Red);
    Color p = c.get_pixel(-1, -1);
    EXPECT_EQ(p.a, 0); // out of bounds returns transparent
}

// ===================================================================
// Canvas – drawing primitives
// ===================================================================

TEST(CanvasDrawTest, DrawLine) {
    Canvas c(20, 20, Colors::White);
    c.draw_line(0, 0, 19, 0, Colors::Black);
    // First pixel of line should be black
    EXPECT_EQ(c.get_pixel(0, 0), Colors::Black);
    EXPECT_EQ(c.get_pixel(10, 0), Colors::Black);
    // Below the line should still be white
    EXPECT_EQ(c.get_pixel(10, 10), Colors::White);
}

TEST(CanvasDrawTest, DrawRectFilled) {
    Canvas c(20, 20, Colors::White);
    c.draw_rect(2, 2, 5, 5, Colors::Green, true);
    EXPECT_EQ(c.get_pixel(4, 4), Colors::Green);
    EXPECT_EQ(c.get_pixel(0, 0), Colors::White);
}

TEST(CanvasDrawTest, DrawRectOutline) {
    Canvas c(20, 20, Colors::White);
    c.draw_rect(2, 2, 10, 10, Colors::Red, false);
    // Border pixels should be red
    EXPECT_EQ(c.get_pixel(2, 2), Colors::Red);
    EXPECT_EQ(c.get_pixel(11, 2), Colors::Red);
    // Interior should still be white
    EXPECT_EQ(c.get_pixel(6, 6), Colors::White);
}

TEST(CanvasDrawTest, DrawCircleFilled) {
    Canvas c(30, 30, Colors::White);
    c.draw_circle(15, 15, 5, Colors::Blue, true);
    // Center should be blue
    EXPECT_EQ(c.get_pixel(15, 15), Colors::Blue);
    // Far corner should be white
    EXPECT_EQ(c.get_pixel(0, 0), Colors::White);
}

TEST(CanvasDrawTest, DrawText) {
    Canvas c(100, 20, Colors::White);
    c.draw_text(2, 2, "AB", Colors::Black);
    // Some pixels in the glyph region should be black
    // 'A' at (2,2): the top-center area should have a pixel set
    bool found_black = false;
    for (int y = 2; y < 9; ++y)
        for (int x = 2; x < 7; ++x)
            if (c.get_pixel(x, y) == Colors::Black) found_black = true;
    EXPECT_TRUE(found_black);
}

// ===================================================================
// Canvas – BMP export
// ===================================================================

TEST(CanvasExportTest, SaveBMP) {
    Canvas c(10, 10, Colors::Red);
    std::string path = "test_output.bmp";
    EXPECT_TRUE(c.save_bmp(path));

    // Verify file exists and has BMP magic bytes
    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    char magic[2];
    f.read(magic, 2);
    EXPECT_EQ(magic[0], 'B');
    EXPECT_EQ(magic[1], 'M');
    f.close();
    std::remove(path.c_str());
}

TEST(CanvasExportTest, SavePNG) {
    Canvas c(10, 10, Colors::Green);
    std::string path = "test_output.png";
    EXPECT_TRUE(c.save_png(path));

    // Verify file exists and has PNG signature
    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    uint8_t sig[4];
    f.read(reinterpret_cast<char*>(sig), 4);
    EXPECT_EQ(sig[0], 0x89);
    EXPECT_EQ(sig[1], 'P');
    EXPECT_EQ(sig[2], 'N');
    EXPECT_EQ(sig[3], 'G');
    f.close();
    std::remove(path.c_str());
}

TEST(CanvasExportTest, SaveJPG) {
    Canvas c(10, 10, Colors::Blue);
    std::string path = "test_output.jpg";
    EXPECT_TRUE(c.save_jpg(path, 90));

    // Verify file exists and has JPEG magic bytes (FFD8)
    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    uint8_t sig[2];
    f.read(reinterpret_cast<char*>(sig), 2);
    EXPECT_EQ(sig[0], 0xFF);
    EXPECT_EQ(sig[1], 0xD8);
    f.close();
    std::remove(path.c_str());
}

// ===================================================================
// Graph – construction and rendering
// ===================================================================

TEST(GraphTest, NoSeriesThrows) {
    Graph g;
    EXPECT_THROW(g.render(), std::runtime_error);
}

TEST(GraphTest, MismatchedXYThrows) {
    Graph g;
    DataSeries s;
    s.x_values = {1, 2, 3};
    s.y_values = {10, 20};
    EXPECT_THROW(g.add_series(s), std::invalid_argument);
}

TEST(GraphTest, LineGraphRenders) {
    Graph g(200, 150, GraphType::Line);
    g.set_title("Test");
    g.set_x_label("X");
    g.set_y_label("Y");
    g.add_series({"data", {1,2,3,4,5}, {10,20,15,25,30}, Colors::Blue});
    Canvas c = g.render();
    EXPECT_EQ(c.width(), 200);
    EXPECT_EQ(c.height(), 150);
    // Should be exportable
    EXPECT_TRUE(c.save_bmp("test_line_graph.bmp"));
    std::remove("test_line_graph.bmp");
}

TEST(GraphTest, BarGraphRenders) {
    Graph g(200, 150, GraphType::Bar);
    g.add_series({"scores", {1,2,3}, {85,92,78}, Colors::Green});
    Canvas c = g.render();
    EXPECT_EQ(c.width(), 200);
    EXPECT_EQ(c.height(), 150);
}

TEST(GraphTest, ScatterGraphRenders) {
    Graph g(200, 150, GraphType::Scatter);
    g.add_series({"pts", {1,3,5,7}, {2,4,1,6}, Colors::Red});
    Canvas c = g.render();
    EXPECT_EQ(c.width(), 200);
}

TEST(GraphTest, MultipleSeriesRender) {
    Graph g(300, 200, GraphType::Line);
    g.add_series({"train", {1,2,3}, {0.9,0.5,0.2}, Colors::Blue});
    g.add_series({"valid", {1,2,3}, {0.95,0.6,0.3}, Colors::Red});
    Canvas c = g.render();
    EXPECT_EQ(c.width(), 300);
}

// ===================================================================
// Table – construction and rendering
// ===================================================================

TEST(TableTest, EmptyTableThrows) {
    Table t;
    EXPECT_THROW(t.render(), std::runtime_error);
}

TEST(TableTest, BasicTableRenders) {
    Table t;
    t.set_headers({"Name", "Score", "Grade"});
    t.add_row({"Alice", "95", "A"});
    t.add_row({"Bob",   "82", "B"});
    t.add_row({"Carol", "91", "A"});
    Canvas c = t.render();
    EXPECT_GT(c.width(), 0);
    EXPECT_GT(c.height(), 0);
    EXPECT_TRUE(c.save_bmp("test_table.bmp"));
    std::remove("test_table.bmp");
}

TEST(TableTest, TableWithoutHeaders) {
    Table t;
    t.add_row({"foo", "bar"});
    t.add_row({"baz", "qux"});
    Canvas c = t.render();
    EXPECT_GT(c.width(), 0);
}

TEST(TableTest, TableExportPNG) {
    Table t;
    t.set_headers({"Col1", "Col2"});
    t.add_row({"Hello", "World"});
    Canvas c = t.render();
    EXPECT_TRUE(c.save_png("test_table.png"));
    std::remove("test_table.png");
}
