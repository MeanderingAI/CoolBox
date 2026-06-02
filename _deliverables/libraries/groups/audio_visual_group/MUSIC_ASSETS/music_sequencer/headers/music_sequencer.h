#ifndef TREKKER_MUSIC_ASSETS_MUSIC_SEQUENCER_H
#define TREKKER_MUSIC_ASSETS_MUSIC_SEQUENCER_H

#include "note_synthesis.h"
#include "music_theory.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace trekker {
namespace sequencer {

// ── Step event ────────────────────────────────────────────────────────────────
// Represents one step in a pattern grid.

struct StepEvent {
    bool  active       = false;
    int   midi_note    = 60;
    float velocity     = 0.8f;    // [0.0, 1.0]  — slider-controlled
    float gate         = 0.5f;    // note-on fraction of step duration [0.0, 1.0]
    float probability  = 1.0f;    // probability this step fires [0.0, 1.0]
};

// ── Pattern ───────────────────────────────────────────────────────────────────
// A fixed-length step pattern on one track.

class Pattern {
public:
    Pattern(std::size_t num_steps = 16, float beats_per_step = 0.25f);

    std::size_t num_steps()        const { return steps_.size(); }
    float       beats_per_step()   const { return beats_per_step_; }
    float       total_beats()      const { return num_steps() * beats_per_step_; }

    StepEvent&       step(std::size_t idx)       { return steps_[idx]; }
    const StepEvent& step(std::size_t idx) const { return steps_[idx]; }

    // Enable/disable a step.
    void toggle(std::size_t idx) { steps_[idx].active = !steps_[idx].active; }

    // Set velocity for all active steps.
    void set_global_velocity(float v);

    // Transpose all active steps by semitones.
    void transpose(int semitones);

    // Quantise active steps to the given scale.
    void quantise_to_scale(const music_theory::Scale& scale);

    void set_num_steps(std::size_t n);
    void set_beats_per_step(float bps) { beats_per_step_ = bps; }

private:
    std::vector<StepEvent> steps_;
    float                  beats_per_step_;
};

// ── MixerBus ──────────────────────────────────────────────────────────────────
// Per-track slider parameters consumed by the sequencer's audio render.

struct TrackParams {
    float volume       = 1.0f;   // [0.0, 1.0]  — fader slider
    float pan          = 0.0f;   // [-1.0, 1.0] — pan slider
    bool  muted        = false;
    bool  soloed       = false;
    music::AdsrParams  adsr;
    music::OscillatorParams oscillator;
};

class MixerBus {
public:
    explicit MixerBus(std::size_t num_tracks);

    TrackParams&       track(std::size_t i);
    const TrackParams& track(std::size_t i) const;
    std::size_t        num_tracks() const { return tracks_.size(); }

    // Master volume slider [0.0, 2.0].
    void  set_master_volume(float v);
    float master_volume() const;

    // Master tempo (also available on Sequencer — kept in sync).
    void  set_bpm(float bpm);
    float bpm() const;

private:
    std::vector<TrackParams> tracks_;
    float                    master_volume_ = 1.f;
    float                    bpm_           = 120.f;
    mutable std::mutex       mtx_;
};

// ── Sequencer ─────────────────────────────────────────────────────────────────
// Combines N tracks (patterns + mixer params) into a real-time audio renderer.
// Slider parameters (BPM, velocity, etc.) can be changed at any time.

class Sequencer {
public:
    // Callback fired on every step event: (track_idx, step_idx, event).
    using StepCallback = std::function<void(std::size_t, std::size_t, const StepEvent&)>;

    struct Config {
        std::uint32_t sample_rate  = 48000;
        std::size_t   num_tracks   = 4;
        std::size_t   steps_per_track = 16;
        float         bpm          = 120.f;
        float         beats_per_step = 0.25f; // 1/16 note at 4/4
    };

    Sequencer();
    explicit Sequencer(Config cfg);

    // ── Slider controls (thread-safe) ──────────────────────────────────────
    void set_bpm(float bpm);
    void set_master_volume(float v);
    void set_track_volume(std::size_t track, float v);
    void set_track_pan(std::size_t track, float pan);
    void set_track_velocity(std::size_t track, float v); // sets all steps
    void set_track_mute(std::size_t track, bool muted);
    void set_track_solo(std::size_t track, bool soloed);
    void set_step_active(std::size_t track, std::size_t step, bool active);
    void set_step_note(std::size_t track, std::size_t step, int midi_note);
    void set_step_velocity(std::size_t track, std::size_t step, float v);
    void set_step_gate(std::size_t track, std::size_t step, float gate);
    void set_adsr(std::size_t track, const music::AdsrParams& p);
    void set_oscillator(std::size_t track, const music::OscillatorParams& p);

    // ── Playback ──────────────────────────────────────────────────────────
    void play();
    void pause();
    void stop();
    bool is_playing() const { return playing_.load(); }

    // Render `num_samples` of stereo (interleaved L,R) audio.
    // Returns the number of samples actually rendered.
    std::size_t render(std::vector<float>& stereo_out, std::size_t num_samples);

    // ── Pattern access ────────────────────────────────────────────────────
    Pattern&       pattern(std::size_t track)       { return patterns_[track]; }
    const Pattern& pattern(std::size_t track) const { return patterns_[track]; }
    MixerBus&      mixer()       { return bus_; }
    const MixerBus& mixer() const { return bus_; }

    // ── Callback ──────────────────────────────────────────────────────────
    void set_step_callback(StepCallback cb) { step_cb_ = std::move(cb); }

    float bpm() const { return bpm_.load(); }
    double position_beats() const { return position_beats_; }

private:
    void advance_step(std::size_t track);
    float samples_per_step() const;

    Config                    cfg_;
    std::vector<Pattern>      patterns_;
    MixerBus                  bus_;
    std::atomic<float>        bpm_;
    std::atomic<bool>         playing_{ false };
    double                    position_beats_    = 0.0;
    double                    sample_accumulator_= 0.0;
    std::vector<std::size_t>  current_step_;
    std::vector<std::unique_ptr<music::NoteGenerator>> active_notes_;
    StepCallback              step_cb_;
    mutable std::mutex        render_mtx_;
};

} // namespace sequencer
} // namespace trekker

#endif // TREKKER_MUSIC_ASSETS_MUSIC_SEQUENCER_H
