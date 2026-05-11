#include "video_display.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace app_assets::video_display;

static int passed = 0, failed = 0;

#define EXPECT_TRUE(expr) \
    do { if (!(expr)) { std::cerr << "FAIL: " #expr << "\n"; ++failed; } else { ++passed; } } while(0)
#define EXPECT_EQ(a,b) \
    do { if ((a)!=(b)) { std::cerr << "FAIL: " #a " == " #b " (" << (a) << " vs " << (b) << ")\n"; ++failed; } else { ++passed; } } while(0)

// ── FrameBuffer tests ─────────────────────────────────────────────────────────

static void test_framebuffer_construction() {
    FrameBuffer fb(320, 240);
    EXPECT_EQ(fb.width(),  std::size_t(320));
    EXPECT_EQ(fb.height(), std::size_t(240));
    EXPECT_EQ(fb.stride(), std::size_t(1280));   // 320 * 4 RGBA bytes
    EXPECT_EQ(fb.raw_data().size(), std::size_t(320 * 240 * 4));
}

static void test_framebuffer_clear_and_pixel() {
    FrameBuffer fb(4, 4);
    fb.clear({10, 20, 30, 255});
    const Rgba px = fb.pixel(2, 2);
    EXPECT_EQ(px.r, std::uint8_t(10));
    EXPECT_EQ(px.g, std::uint8_t(20));
    EXPECT_EQ(px.b, std::uint8_t(30));
    EXPECT_EQ(px.a, std::uint8_t(255));
}

static void test_framebuffer_set_pixel() {
    FrameBuffer fb(8, 8);
    fb.clear({0, 0, 0, 255});
    fb.set_pixel(3, 5, {100, 150, 200, 255});
    const Rgba px = fb.pixel(3, 5);
    EXPECT_EQ(px.r, std::uint8_t(100));
    EXPECT_EQ(px.g, std::uint8_t(150));
    EXPECT_EQ(px.b, std::uint8_t(200));
}

static void test_framebuffer_blit_rgb24() {
    // 2×2 RGB24 source (red, green, blue, white).
    std::uint8_t src[12] = {
        255, 0,   0,    0, 255, 0,
        0,   0,   255,  255, 255, 255
    };
    FrameBuffer fb(2, 2);
    fb.blit_rgb24(src, 2, 2);
    EXPECT_EQ(fb.pixel(0,0).r, std::uint8_t(255));
    EXPECT_EQ(fb.pixel(0,0).g, std::uint8_t(0));
    EXPECT_EQ(fb.pixel(1,0).g, std::uint8_t(255));
    EXPECT_EQ(fb.pixel(0,1).b, std::uint8_t(255));
    EXPECT_EQ(fb.pixel(1,1).r, std::uint8_t(255));
    EXPECT_EQ(fb.pixel(1,1).g, std::uint8_t(255));
}

static void test_framebuffer_resize() {
    FrameBuffer fb(100, 100);
    fb.resize(640, 480);
    EXPECT_EQ(fb.width(),  std::size_t(640));
    EXPECT_EQ(fb.height(), std::size_t(480));
    EXPECT_EQ(fb.raw_data().size(), std::size_t(640 * 480 * 4));
}

// ── Overlay layout tests ──────────────────────────────────────────────────────

static void test_overlay_layout_topleft() {
    Overlay ov;
    ov.text      = "Hello";    // 5 chars
    ov.anchor    = OverlayAnchor::TopLeft;
    ov.offset_x  = 4;
    ov.offset_y  = 4;
    ov.visible   = true;

    auto cmds = layout_overlays({ov}, 640, 480, 8, 16);
    EXPECT_EQ(cmds.size(), std::size_t(1));
    EXPECT_EQ(cmds[0].x, 4);
    EXPECT_EQ(cmds[0].y, 4);
    EXPECT_EQ(cmds[0].w, 5 * 8);   // 40
    EXPECT_EQ(cmds[0].h, 16);
}

static void test_overlay_layout_bottomright() {
    Overlay ov;
    ov.text      = "TC";        // 2 chars  → w=16
    ov.anchor    = OverlayAnchor::BottomRight;
    ov.offset_x  = 8;
    ov.offset_y  = 8;
    ov.visible   = true;

    auto cmds = layout_overlays({ov}, 640, 480, 8, 16);
    EXPECT_EQ(cmds.size(), std::size_t(1));
    EXPECT_EQ(cmds[0].x, 640 - 16 - 8);  // 616
    EXPECT_EQ(cmds[0].y, 480 - 16 - 8);  // 456
}

static void test_overlay_invisible_skipped() {
    Overlay ov;
    ov.text    = "Hidden";
    ov.visible = false;
    auto cmds = layout_overlays({ov}, 640, 480);
    EXPECT_EQ(cmds.size(), std::size_t(0));
}

// ── VideoPlayer tests ─────────────────────────────────────────────────────────

static void test_player_initial_state() {
    VideoPlayer p;
    EXPECT_TRUE(p.state() == PlaybackState::Stopped);
    EXPECT_EQ(p.position_us(), std::int64_t(0));
}

static void test_player_play_pause_stop() {
    VideoPlayer p;
    p.play();
    EXPECT_TRUE(p.state() == PlaybackState::Playing);
    p.pause();
    EXPECT_TRUE(p.state() == PlaybackState::Paused);
    p.stop();
    EXPECT_TRUE(p.state() == PlaybackState::Stopped);
    EXPECT_EQ(p.position_us(), std::int64_t(0));
}

static void test_player_seek() {
    VideoPlayer p;
    p.seek(500'000);
    EXPECT_EQ(p.position_us(), std::int64_t(500'000));
    p.seek(-100);  // clamped to 0
    EXPECT_EQ(p.position_us(), std::int64_t(0));
}

static void test_player_tick_advances_position() {
    PlayerConfig cfg;
    cfg.fps          = 30.0;
    cfg.duration_us  = 10'000'000;  // 10 s
    VideoPlayer p(cfg);
    p.play();

    bool called = false;
    p.set_frame_provider([&](std::int64_t, FrameBuffer&) { called = true; });

    FrameBuffer fb(128, 72);
    // One full frame period (33 333 µs) should trigger exactly one frame.
    const bool updated = p.tick(33'333, fb);
    EXPECT_TRUE(updated);
    EXPECT_TRUE(called);
    EXPECT_TRUE(p.position_us() > 0);
}

static void test_player_volume_clamped() {
    VideoPlayer p;
    p.set_volume(2.0f);
    EXPECT_EQ(p.volume(), 1.f);
    p.set_volume(-0.5f);
    EXPECT_EQ(p.volume(), 0.f);
}

static void test_player_rate_clamped() {
    VideoPlayer p;
    p.set_playback_rate(10.0);
    EXPECT_EQ(p.playback_rate(), 8.0);
    p.set_playback_rate(-1.0);
    EXPECT_EQ(p.playback_rate(), 0.0);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    test_framebuffer_construction();
    test_framebuffer_clear_and_pixel();
    test_framebuffer_set_pixel();
    test_framebuffer_blit_rgb24();
    test_framebuffer_resize();

    test_overlay_layout_topleft();
    test_overlay_layout_bottomright();
    test_overlay_invisible_skipped();

    test_player_initial_state();
    test_player_play_pause_stop();
    test_player_seek();
    test_player_tick_advances_position();
    test_player_volume_clamped();
    test_player_rate_clamped();

    std::cout << passed << " passed, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
