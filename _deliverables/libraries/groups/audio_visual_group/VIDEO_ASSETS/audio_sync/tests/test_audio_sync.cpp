#include "../headers/audio_sync.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))
#define EXPECT_NEAR(a,b,eps) EXPECT_TRUE(std::abs((double)(a)-(double)(b)) < (eps))

#include <cmath>

int main() {
    using namespace trekker::av_sync;

    // AvTimestamp round-trip
    {
        auto ts = AvTimestamp::from_pts(500'000);
        EXPECT_EQ(ts.pts, std::int64_t(500'000));
        EXPECT_EQ(ts.pts_ms(), std::int64_t(500));
    }

    // MasterClock tick
    {
        MasterClock clk;
        clk.set_time_us(0);
        clk.tick(33'333);
        EXPECT_EQ(clk.time_us(), std::int64_t(33'333));
    }

    // SyncController: no drift → correction_factor = 1.0
    {
        SyncController sc;
        MasterClock clk;
        clk.set_time_us(100'000);
        sc.report_audio_pts(100'000, clk);  // drift = 0
        EXPECT_NEAR(sc.correction_factor(), 1.0f, 1e-5f);
        EXPECT_EQ(sc.drift_us(), std::int64_t(0));
    }

    // SyncController: large positive drift → factor < 1 (slow down audio)
    {
        SyncController sc;
        MasterClock clk;
        clk.set_time_us(0);
        // Fill history with 100ms of audio ahead
        for (int i = 0; i < 40; ++i)
            sc.report_audio_pts(100'000, clk);
        EXPECT_TRUE(sc.correction_factor() < 1.0f);
    }

    // LipSyncAdjuster: adjust + should_present
    {
        LipSyncAdjuster lsa;
        lsa.set_offset_ms(100);  // delay audio by 100ms
        EXPECT_EQ(lsa.offset_ms(), std::int64_t(100));
        const std::int64_t raw_pts = 200'000;
        const std::int64_t adj = lsa.adjust_audio_pts(raw_pts);
        EXPECT_EQ(adj, std::int64_t(100'000));

        MasterClock clk;
        clk.set_time_us(100'000);  // master at 100ms
        EXPECT_TRUE(lsa.should_present(adj, clk, 20'000));
    }

    // DriftDetector: zero drift → rate ≈ 0
    {
        DriftDetector dd(16);
        for (int i = 0; i < 10; ++i)
            dd.push(i * 40'000, i * 40'000);  // audio == video
        EXPECT_NEAR(dd.drift_rate_us_per_s(), 0.0, 10.0);
        EXPECT_EQ(dd.current_offset_us(), std::int64_t(0));
    }

    return failed > 0 ? 1 : 0;
}
