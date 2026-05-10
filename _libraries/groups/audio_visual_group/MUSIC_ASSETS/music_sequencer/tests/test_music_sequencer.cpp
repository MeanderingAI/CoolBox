#include "../headers/music_sequencer.h"
#include <cmath>

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))
#define EXPECT_NEAR(a,b,eps) EXPECT_TRUE(std::abs((double)(a)-(double)(b)) < (eps))

int main() {
    using namespace trekker::sequencer;

    // Sequencer construction
    {
        Sequencer::Config cfg;
        cfg.num_tracks = 2; cfg.steps_per_track = 8; cfg.bpm = 120.f;
        Sequencer seq(cfg);
        EXPECT_NEAR(seq.bpm(), 120.f, 1e-4f);
        EXPECT_EQ(seq.pattern(0).num_steps(), std::size_t(8));
    }

    // BPM slider
    {
        Sequencer seq;
        seq.set_bpm(140.f);
        EXPECT_NEAR(seq.bpm(), 140.f, 1e-4f);
        // Out of range clamped
        seq.set_bpm(0.f);
        EXPECT_TRUE(seq.bpm() >= 1.f);
    }

    // Pattern step toggle + velocity
    {
        Sequencer seq;
        seq.set_step_active(0, 0, true);
        seq.set_step_note(0, 0, 60);
        seq.set_step_velocity(0, 0, 0.9f);
        EXPECT_TRUE(seq.pattern(0).step(0).active);
        EXPECT_EQ(seq.pattern(0).step(0).midi_note, 60);
        EXPECT_NEAR(seq.pattern(0).step(0).velocity, 0.9f, 1e-5f);
    }

    // Render while stopped → silent
    {
        Sequencer seq;
        std::vector<float> out;
        seq.render(out, 256);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_NEAR(peak, 0.f, 1e-9f);
    }

    // Render while playing with one active step → non-silent
    {
        Sequencer::Config cfg; cfg.num_tracks = 1; cfg.steps_per_track = 1; cfg.bpm = 60.f;
        Sequencer seq(cfg);
        seq.set_step_active(0, 0, true);
        seq.set_step_note(0, 0, 69); // A4
        seq.play();
        // Render enough samples for one full step at 60 BPM (1 beat = 1s = 48000 samples).
        std::vector<float> out;
        seq.render(out, 48000);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_TRUE(peak > 0.f);
    }

    // MixerBus: volume + pan setters
    {
        Sequencer seq;
        seq.set_track_volume(0, 0.5f);
        EXPECT_NEAR(seq.mixer().track(0).volume, 0.5f, 1e-5f);
        seq.set_track_pan(0, -1.f);
        EXPECT_NEAR(seq.mixer().track(0).pan, -1.f, 1e-5f);
    }

    // Mute: muted track produces silence alongside unmuted
    {
        Sequencer::Config cfg; cfg.num_tracks = 1; cfg.steps_per_track = 1; cfg.bpm = 60.f;
        Sequencer seq(cfg);
        seq.set_step_active(0, 0, true);
        seq.set_track_mute(0, true);
        seq.play();
        std::vector<float> out;
        seq.render(out, 48000);
        float peak = 0.f;
        for (float s : out) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_NEAR(peak, 0.f, 1e-9f);
    }

    return failed > 0 ? 1 : 0;
}
