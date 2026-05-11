#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace music_studio {

// ── TheoryView ────────────────────────────────────────────────────────────────
// Music-theory panel backed by music_theory (when linked).
//
// Features:
//   • Root note selector (C .. B)
//   • Scale type selector (Major / Minor / Dorian / Phrygian / … 15 types)
//   • Display of scale notes as piano-roll highlights
//   • Chord type selector and voicing display
//   • Chord progression builder (up to 8 chords)
//   • Transpose utility (+/- semitones)
//   • Interval analysis display

class TheoryView {
public:
    TheoryView();
    ~TheoryView();

    void init();
    void tick(std::int64_t delta_us);

    // ── Scale ─────────────────────────────────────────────────────────────
    void set_root(int pitch_class);     // 0=C .. 11=B
    void set_scale_type(int scale_type);
    int  root()       const;
    int  scale_type() const;

    // Returns MIDI notes in one octave (relative to root) for the current scale.
    std::vector<int> scale_notes() const;

    // ── Chord ─────────────────────────────────────────────────────────────
    void set_chord_type(int chord_type);
    int  chord_type() const;
    // Returns MIDI intervals for the current chord (root = 0).
    std::vector<int> chord_intervals() const;

    // ── Progression ───────────────────────────────────────────────────────
    void add_chord_to_progression(int root_offset, int chord_type);
    void clear_progression();
    const std::vector<std::pair<int,int>>& progression() const;

    // ── Utility ───────────────────────────────────────────────────────────
    void set_transpose(int semitones);  // applied on export
    int  transpose() const;
    std::string current_scale_name() const;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace music_studio
