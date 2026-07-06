#include "../headers/note_synthesis.h"
#include <cmath>

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))
#define EXPECT_NEAR(a,b,eps) EXPECT_TRUE(std::abs((double)(a)-(double)(b)) < (eps))

int main() {
    using namespace trekker::music;

    // MIDI <-> frequency
    EXPECT_NEAR(midi_to_hz(69), 440.f, 0.001f);
    EXPECT_NEAR(midi_to_hz(57), 220.f, 0.001f);

    // Note name parsing
    EXPECT_EQ(note_name_to_midi("A4"), 69);
    EXPECT_EQ(note_name_to_midi("C4"), 60);
    EXPECT_EQ(note_name_to_midi("C#4"), 61);
    EXPECT_EQ(note_name_to_midi("Bb3"), 46);

    // ADSR envelope: attack phase increases from 0
    {
        AdsrParams p; p.attack_s = 0.1f; p.decay_s = 0.1f;
        p.sustain = 0.7f; p.release_s = 0.1f;
        AdsrEnvelope env(p, 48000);
        env.note_on();
        const float first = env.next_sample();
        const float later = env.next_sample();
        EXPECT_TRUE(first >= 0.f && later > first);
    }

    // ADSR envelope: done after release
    {
        AdsrParams p; p.attack_s = 0.001f; p.decay_s = 0.001f;
        p.sustain = 0.5f; p.release_s = 0.001f;
        AdsrEnvelope env(p, 48000);
        env.note_on();
        for (int i = 0; i < 300; ++i) env.next_sample();
        env.note_off();
        for (int i = 0; i < 300; ++i) env.next_sample();
        EXPECT_TRUE(env.is_done());
    }

    // Oscillator: sine output is in [-1, 1]
    {
        OscillatorParams p; p.waveform = Waveform::Sine; p.frequency = 440.f; p.amplitude = 1.f;
        Oscillator osc(p, 48000);
        std::vector<float> buf;
        osc.render(buf, 512);
        bool in_range = true;
        for (float s : buf) if (s < -1.01f || s > 1.01f) { in_range = false; break; }
        EXPECT_TRUE(in_range);
    }

    // Oscillator waveforms: sawtooth, triangle, square all produce output
    {
        for (auto wf : { Waveform::Sawtooth, Waveform::Triangle, Waveform::Square,
                         Waveform::Noise, Waveform::Pulse }) {
            OscillatorParams p; p.waveform = wf; p.frequency = 220.f; p.amplitude = 0.5f;
            Oscillator osc(p, 48000);
            std::vector<float> buf;
            osc.render(buf, 64);
            float rms = 0.f;
            for (float s : buf) rms += s * s;
            rms = std::sqrt(rms / 64);
            EXPECT_TRUE(rms > 0.f);
        }
    }

    // NoteGenerator: generates non-silent audio after note_on
    {
        NoteParams np;
        np.midi_note = 60; np.sample_rate = 48000; np.velocity = 1.f;
        np.adsr.attack_s = 0.01f; np.adsr.sustain = 0.8f;
        NoteGenerator gen(np);
        gen.note_on();
        std::vector<float> buf;
        gen.generate(buf, 512);
        float peak = 0.f;
        for (float s : buf) if (std::abs(s) > peak) peak = std::abs(s);
        EXPECT_TRUE(peak > 0.f);
    }

    return failed > 0 ? 1 : 0;
}
