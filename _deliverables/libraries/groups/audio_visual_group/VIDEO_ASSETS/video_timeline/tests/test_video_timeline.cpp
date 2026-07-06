#include "../headers/video_timeline.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))

int main() {
    using namespace trekker::timeline;

    // Add a clip and retrieve it by PTS.
    {
        Timeline tl;
        auto& track = tl.add_track(TrackType::Video, "V1");
        Clip c;
        c.id = 1; c.source_path = "foo.mp4";
        c.in_point_us = 0; c.out_point_us = 5'000'000;
        c.position_us = 0;
        track.add_clip(c);
        EXPECT_TRUE(track.clip_at(2'500'000) != nullptr);
        EXPECT_TRUE(track.clip_at(6'000'000) == nullptr);
    }

    // Timeline duration is driven by the longest track.
    {
        Timeline tl;
        auto& v = tl.add_track(TrackType::Video, "V1");
        auto& a = tl.add_track(TrackType::Audio, "A1");
        Clip cv; cv.id=1; cv.in_point_us=0; cv.out_point_us=10'000'000; cv.position_us=0;
        Clip ca; ca.id=2; ca.in_point_us=0; ca.out_point_us= 8'000'000; ca.position_us=0;
        v.add_clip(cv); a.add_clip(ca);
        EXPECT_EQ(tl.duration_us(), std::int64_t(10'000'000));
    }

    // Split clip produces two clips.
    {
        Timeline tl;
        auto& tr = tl.add_track(TrackType::Video, "V1");
        Clip c; c.id=1; c.in_point_us=0; c.out_point_us=10'000'000; c.position_us=0;
        tr.add_clip(c);
        EXPECT_TRUE(tl.split_clip(tr.id(), 5'000'000));
        EXPECT_EQ(tr.clips().size(), std::size_t(2));
        EXPECT_EQ(tr.clips()[0].timeline_end_us(), std::int64_t(5'000'000));
        EXPECT_EQ(tr.clips()[1].position_us,       std::int64_t(5'000'000));
    }

    // Remove clip.
    {
        Track tr(1, TrackType::Audio, "A1");
        Clip c; c.id=42; c.in_point_us=0; c.out_point_us=1'000'000; c.position_us=0;
        tr.add_clip(c);
        EXPECT_TRUE(tr.remove_clip(42));
        EXPECT_TRUE(tr.clips().empty());
        EXPECT_TRUE(!tr.remove_clip(42)); // already removed
    }

    // EDL export contains TRACK and CLIP keywords.
    {
        Timeline tl;
        auto& tr = tl.add_track(TrackType::Video, "Main");
        Clip c; c.id=1; c.source_path="media.mp4"; c.in_point_us=0; c.out_point_us=3'000'000; c.position_us=0;
        tr.add_clip(c);
        const std::string edl = tl.export_edl();
        EXPECT_TRUE(edl.find("TRACK") != std::string::npos);
        EXPECT_TRUE(edl.find("media.mp4") != std::string::npos);
    }

    return failed > 0 ? 1 : 0;
}
