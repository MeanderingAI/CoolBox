#ifndef TREKKER_MUSIC_ASSETS_NOTE_SYNTHESIS_H
#define TREKKER_MUSIC_ASSETS_NOTE_SYNTHESIS_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cmath>
#include <string>

namespace trekker {
namespace music {

// ── Musical note helpers ──────────────────────────────────────────────────────

// MIDI note number → frequency in Hz (A4 = MIDI 69 = 440 Hz).
inline float midi_to_hz(int midi_note) {
    return 440.f * std::pow(2.f, (midi_note - 69) / 12.f);
}

// Scientific pitch name ("A4", "C#3", ...) → MIDI note number.
// Returns -1 on parse failure.
int note_name_to_midi(const std::string& name);

// ── Waveform types ────────────────────────────────────────────────────────────

enum class Waveform {
    Sine,
    Square,
    Sawtooth,
    Triangle,
    Noise,          // white noise (seeded per oscillator)
    Pulse,          // square with adjustable duty cycle
};

// ── ADSR Envelope ─────────────────────────────────────────────────────────────
// All durations are in seconds.  Levels are in [0.0, 1.0].

struct AdsrParams {
    float attack_s   = 0.01f;   // time to reach peak after note on
    float decay_s    = 0.10f;   // time to fall from peak to sustain level
    float sustain    = 0.70f;   // level held while key is pressed (0–1)
    float release_s  = 0.20f;   // time to silence after note off

    // Clamp all values to valid ranges.
    AdsrParams clamped() const;
};

class AdsrEnvelope {
public:
    explicit AdsrEnvelope(AdsrParams p, std::uint32_t sample_rate);

    // Call once when note starts.
    void note_on();
    // Call once when key is released; envelope continues through release phase.
    void note_off();

    // Advance one sample and return the current amplitude multiplier [0, 1].
    float next_sample();

    bool is_done() const { return stage_ == Stage::Done; }
    void reset();

    // Slider setters (live updates while playing).
    void set_attack(float s);
    void set_decay(float s);
    void set_sustain(float level);
    void set_release(float s);

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release, Done };
    AdsrParams      p_;
    std::uint32_t   sample_rate_;
    Stage           stage_ = Stage::Idle;
    float           level_ = 0.f;
    std::uint64_t   sample_counter_ = 0;
    std::uint64_t   stage_start_    = 0;
};

// ── Oscillator ────────────────────────────────────────────────────────────────
// Single-channel waveform generator with tunable parameters.

struct OscillatorParams {
    Waveform  waveform    = Waveform::Sine;
    float     frequency   = 440.f;    // Hz
    float     amplitude   = 0.8f;     // [0.0, 1.0]
    float     detune_cents= 0.f;      // fine-tune ± in cents (100 cents = 1 semitone)
    float     phase_rad   = 0.f;      // initial phase offset
    float     duty_cycle  = 0.5f;     // for Pulse waveform (0.0–1.0)
};

class Oscillator {
public:
    explicit Oscillator(OscillatorParams p, std::uint32_t sample_rate);

    // Render `count` samples into `out` (additive — values are added, not overwritten).
    void render(std::vector<float>& out, std::size_t count);

    // Slider setters (thread-safe if called between render() calls).
    void set_waveform(Waveform w)    { p_.waveform     = w; }
    void set_frequency(float hz)    { p_.frequency    = hz; update_increment(); }
    void set_amplitude(float a)     { p_.amplitude    = a;  }
    void set_detune_cents(float c)  { p_.detune_cents = c;  update_increment(); }
    void set_duty_cycle(float d)    { p_.duty_cycle   = d;  }

    void reset();

private:
    void update_increment();
    float next_sample();

    OscillatorParams p_;
    std::uint32_t    sample_rate_;
    double           phase_     = 0.0;   // accumulated phase [0, 2π)
    double           increment_ = 0.0;   // phase increment per sample
    std::uint32_t    noise_seed_= 12345;
};

// ── NoteGenerator ─────────────────────────────────────────────────────────────
// Combines multiple Oscillators with an AdsrEnvelope to produce one note.
// Call generate() repeatedly to pull audio samples.

struct NoteParams {
    int             midi_note    = 69;   // A4
    std::uint32_t   sample_rate  = 48000;
    float           duration_s   = 1.0f; // 0 = held until note_off()
    float           velocity     = 1.0f; // [0.0, 1.0] overall amplitude
    AdsrParams      adsr;
    std::vector<OscillatorParams> oscillators = { OscillatorParams{} };
};

class NoteGenerator {
public:
    explicit NoteGenerator(NoteParams p);

    // Fill `out` with exactly `count` mono samples.
    // Returns false once the envelope is done (note has fully released).
    bool generate(std::vector<float>& out, std::size_t count);

    void note_on();
    void note_off();

    // Live slider controls
    void set_velocity(float v)           { params_.velocity = v; }
    void set_adsr(const AdsrParams& a);
    void set_oscillator_param(std::size_t idx, const OscillatorParams& p);

    bool is_done() const { return envelope_.is_done(); }

private:
    NoteParams               params_;
    AdsrEnvelope             envelope_;
    std::vector<Oscillator>  oscillators_;
};

} // namespace music
} // namespace trekker

#endif // TREKKER_MUSIC_ASSETS_NOTE_SYNTHESIS_H
