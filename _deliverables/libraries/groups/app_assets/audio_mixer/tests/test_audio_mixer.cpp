#include "../headers/audio_mixer.hpp"
#include <cmath>

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))
#define EXPECT_NEAR(a,b,eps) EXPECT_TRUE(std::abs((double)(a)-(double)(b)) < (eps))

int main() {
    using namespace app_assets::audio_mixer;

    // MixerConsole construction
    {
        MixerConsole mixer(4);
        EXPECT_EQ(mixer.num_channels(), std::size_t(4));
    }

    // Add / remove channel
    {
        MixerConsole mixer(0);
        const auto id = mixer.add_channel("Kick");
        EXPECT_EQ(mixer.num_channels(), std::size_t(1));
        EXPECT_TRUE(mixer.channel(id) != nullptr);
        mixer.remove_channel(id);
        EXPECT_EQ(mixer.num_channels(), std::size_t(0));
    }

    // Volume slider clamped to [0,1]
    {
        MixerConsole mixer(1);
        auto id = mixer.channel(0) ? mixer.channel(0)->id() : std::size_t(0);
        mixer.set_channel_volume(id, 2.5f);
        EXPECT_NEAR(mixer.channel(id)->state().volume, 1.f, 1e-5f);
        mixer.set_channel_volume(id, -1.f);
        EXPECT_NEAR(mixer.channel(id)->state().volume, 0.f, 1e-5f);
    }

    // Param callback fires on volume change
    {
        MixerConsole mixer(1);
        bool fired = false;
        mixer.set_param_callback([&](std::size_t, ChannelParam p, float) {
            if (p == ChannelParam::Volume) fired = true;
        });
        const auto id = mixer.channel(0)->id();
        mixer.set_channel_volume(id, 0.5f);
        EXPECT_TRUE(fired);
    }

    // Muted channel produces silence
    {
        MixerConsole mixer(1);
        const auto id = mixer.channel(0)->id();
        mixer.set_channel_mute(id, true);
        const std::vector<float> mono(64, 1.f);
        const auto out = mixer.mix({mono}, 64);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_NEAR(peak, 0.f, 1e-9f);
    }

    // Active channel produces non-silence
    {
        MixerConsole mixer(1);
        const auto id = mixer.channel(0)->id();
        mixer.set_channel_volume(id, 1.f);
        const std::vector<float> mono(64, 0.5f);
        const auto out = mixer.mix({mono}, 64);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_TRUE(peak > 0.f);
    }

    // VuMeter: update then read peak
    {
        VuMeter vu;
        std::vector<float> buf = {0.8f, 0.6f, 0.4f, 0.3f}; // 2 stereo frames
        vu.update(buf);
        const auto s = vu.state();
        EXPECT_NEAR(s.peak_l, 0.8f, 1e-5f);
        EXPECT_NEAR(s.peak_r, 0.6f, 1e-5f);
    }

    // Master limiter clips output
    {
        MixerConsole mixer(2);
        for (std::size_t i = 0; i < mixer.num_channels(); ++i) {
            const auto id = mixer.channel(i)->id();
            mixer.set_channel_volume(id, 1.f);
        }
        mixer.set_master_volume(1.0f);
        mixer.set_limiter_enabled(true);
        mixer.set_limiter_threshold(0.99f);
        const std::vector<float> loud(64, 1.f);
        const auto out = mixer.mix({loud, loud}, 64);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_TRUE(peak <= 0.99f + 1e-5f);
    }

    return failed > 0 ? 1 : 0;
}
