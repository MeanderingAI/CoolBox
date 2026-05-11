#pragma once
#include <cstdint>
#include <string>

namespace music_studio {

// ── SequencerView ─────────────────────────────────────────────────────────────
// Step-sequencer panel:
//   • BPM slider (40 – 300)
//   • Per-track step grid (up to 32 steps × 8 tracks)
//   • Per-step velocity / gate / probability dials
//   • Transport controls (play / pause / stop)
//   • Waveform selector per track (Sine / Square / Saw / Triangle / Pulse)
//   • ADSR sliders per track
//
// When music_sequencer and note_synthesis are linked, SequencerView drives
// Sequencer::render() and hands rendered audio to the mixer bus.

class SequencerView {
public:
    SequencerView();
    ~SequencerView();

    void init();
    void tick(std::int64_t delta_us);  // advance playback + redraw

    // ── Transport controls ────────────────────────────────────────────────
    void set_bpm(double bpm);
    void play();
    void pause();
    void stop();

    // ── Step grid ─────────────────────────────────────────────────────────
    void toggle_step(int track, int step);
    void set_step_note(int track, int step, int midi_note);
    void set_step_velocity(int track, int step, float vel);  // [0,1]
    void set_step_gate(int track, int step, float gate);     // [0,1]

    // ── Per-track parameters ──────────────────────────────────────────────
    void set_track_volume(int track, float vol);
    void set_track_pan(int track, float pan);    // -1 .. +1
    void set_track_muted(int track, bool muted);
    void set_track_waveform(int track, int waveform_enum);
    void set_track_attack(int track, float s);
    void set_track_decay(int track, float s);
    void set_track_sustain(int track, float level);
    void set_track_release(int track, float s);

    double      bpm()         const;
    bool        is_playing()  const;

    static constexpr int kMaxTracks = 8;
    static constexpr int kMaxSteps  = 32;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace music_studio
