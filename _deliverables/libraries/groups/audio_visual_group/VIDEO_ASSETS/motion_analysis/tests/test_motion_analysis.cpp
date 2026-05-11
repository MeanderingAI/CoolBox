#include "../headers/motion_analysis.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))

int main() {
    using namespace trekker::video;
    using namespace trekker::motion;

    // optical_flow: same frames → near-zero flow
    {
        auto frame = VideoFrame::blank(PixelFormat::GRAY8, 32, 32);
        for (auto& v : frame.planes[0].data) v = 128;
        const auto field = optical_flow(frame, frame, 5, 2);
        EXPECT_EQ(field.width,  std::size_t(32));
        EXPECT_EQ(field.height, std::size_t(32));
        EXPECT_TRUE(field.mean_magnitude() < 1.f);
    }

    // estimate_motion_vectors: same frames → all MVs have dx/dy = 0
    {
        auto frame = VideoFrame::blank(PixelFormat::GRAY8, 32, 32);
        for (auto& v : frame.planes[0].data) v = 64;
        const auto mvs = estimate_motion_vectors(frame, frame, 8, 4);
        EXPECT_TRUE(!mvs.empty());
        bool all_zero = true;
        for (const auto& mv : mvs)
            if (mv.dx != 0 || mv.dy != 0) { all_zero = false; break; }
        EXPECT_TRUE(all_zero);
    }

    // detect_scene_cut: identical frames → no cut
    {
        auto a = VideoFrame::blank(PixelFormat::GRAY8, 8, 8);
        for (auto& v : a.planes[0].data) v = 100;
        EXPECT_TRUE(!detect_scene_cut(a, a, 0.30f));
    }

    // detect_scene_cut: black → white → hard cut
    {
        auto black = VideoFrame::blank(PixelFormat::GRAY8, 8, 8);
        for (auto& v : black.planes[0].data) v = 0;
        auto white = VideoFrame::blank(PixelFormat::GRAY8, 8, 8);
        for (auto& v : white.planes[0].data) v = 255;
        EXPECT_TRUE(detect_scene_cut(black, white, 0.30f));
    }

    // SceneDetector: first frame never triggers cut
    {
        SceneDetector sd;
        auto frame = VideoFrame::blank(PixelFormat::GRAY8, 8, 8);
        EXPECT_TRUE(!sd.process(frame));
    }

    // compute_block_activity: returns correct block count
    {
        auto frame = VideoFrame::blank(PixelFormat::GRAY8, 16, 16);
        const auto field = optical_flow(frame, frame, 3, 1);
        const auto activity = compute_block_activity(frame, field, 8);
        EXPECT_EQ(activity.size(), std::size_t(4)); // 2×2 blocks
    }

    return failed > 0 ? 1 : 0;
}
