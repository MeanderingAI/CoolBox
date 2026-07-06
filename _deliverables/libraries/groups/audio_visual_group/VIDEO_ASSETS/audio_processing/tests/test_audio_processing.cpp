#include "../headers/audio_processing.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))
#define EXPECT_NEAR(a,b,eps) EXPECT_TRUE(std::abs((a)-(b)) < (eps))

#include <cmath>
#include <cstdlib>

int main() {
    using namespace trekker::audio;

    // Silent buffer has correct size
    {
        auto buf = AudioBuffer::silent(44100, 2, 1024);
        EXPECT_EQ(buf.num_samples(), std::size_t(1024));
        EXPECT_EQ(buf.num_channels, std::uint32_t(2));
        EXPECT_EQ(buf.sample_rate, std::uint32_t(44100));
        EXPECT_TRUE(buf.planes[0][0] == 0.f);
    }

    // Peak normalise
    {
        auto buf = AudioBuffer::silent(48000, 1, 4);
        buf.planes[0] = {0.1f, 0.5f, -0.3f, 0.2f};
        auto out = peak_normalize(buf, 1.0f);
        float peak = 0.f;
        for (float s : out.planes[0]) peak = std::max(peak, std::abs(s));
        EXPECT_NEAR(peak, 1.0f, 1e-5f);
    }

    // Fade in: first sample should be silent (or near zero)
    {
        auto buf = AudioBuffer::silent(48000, 1, 100);
        for (auto& s : buf.planes[0]) s = 1.0f;
        auto out = fade_in(buf, 50);
        EXPECT_TRUE(out.planes[0][0] < 0.1f);
        EXPECT_NEAR(out.planes[0][99], 1.0f, 1e-5f);
    }

    // Fade out: last sample should be near zero
    {
        auto buf = AudioBuffer::silent(48000, 1, 100);
        for (auto& s : buf.planes[0]) s = 1.0f;
        auto out = fade_out(buf, 50);
        EXPECT_TRUE(out.planes[0][99] < 0.1f);
        EXPECT_NEAR(out.planes[0][0], 1.0f, 1e-5f);
    }

    // Mixer: two identical signals at gain 0.5 each sum to original
    {
        auto a = AudioBuffer::silent(48000, 1, 8);
        for (auto& s : a.planes[0]) s = 0.5f;
        auto b = a.clone();
        auto out = mix({a, b}, {0.5f, 0.5f});
        EXPECT_NEAR(out.planes[0][0], 0.5f, 1e-5f);
    }

    // Resampler: 44100 -> 48000 increases sample count
    {
        auto in = AudioBuffer::silent(44100, 1, 44100);
        for (auto& s : in.planes[0]) s = 0.5f;
        Resampler rs(44100, 48000, 1);
        auto out = rs.process(in);
        EXPECT_TRUE(out.num_samples() > 44100);
    }

    // Stereo→mono channel routing
    {
        auto buf = AudioBuffer::silent(48000, 2, 4);
        buf.planes[0] = {1.f, 1.f, 1.f, 1.f};
        buf.planes[1] = {1.f, 1.f, 1.f, 1.f};
        auto mx  = ChannelRouterMatrix::stereo_to_mono();
        auto out = route_channels(buf, mx);
        EXPECT_EQ(out.num_channels, std::uint32_t(1));
        EXPECT_NEAR(out.planes[0][0], 1.0f, 1e-5f);
    }

    return failed > 0 ? 1 : 0;
}
