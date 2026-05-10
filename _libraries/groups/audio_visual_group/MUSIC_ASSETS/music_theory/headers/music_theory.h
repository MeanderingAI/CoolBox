#ifndef TREKKER_MUSIC_ASSETS_MUSIC_THEORY_H
#define TREKKER_MUSIC_ASSETS_MUSIC_THEORY_H

#include <cstdint>
#include <string>
#include <vector>

namespace trekker {
namespace music_theory {

// ── Pitch class & octave ──────────────────────────────────────────────────────

// A pitch class is a note name without octave information (0 = C, 11 = B).
enum class PitchClass : int {
    C = 0, Cs = 1, D = 2, Ds = 3, E = 4, F = 5,
    Fs = 6, G = 7, Gs = 8, A = 9, As = 10, B = 11
};

const char* pitch_class_name(PitchClass pc);
PitchClass transpose(PitchClass pc, int semitones);

// MIDI note: root_midi_note + scale_degree_semitone.
inline int pitch_to_midi(PitchClass pc, int octave) {
    return 12 * (octave + 1) + static_cast<int>(pc);
}

// ── Intervals ─────────────────────────────────────────────────────────────────

enum class Interval : int {
    Unison            = 0,
    MinorSecond       = 1,
    MajorSecond       = 2,
    MinorThird        = 3,
    MajorThird        = 4,
    PerfectFourth     = 5,
    Tritone           = 6,
    PerfectFifth      = 7,
    MinorSixth        = 8,
    MajorSixth        = 9,
    MinorSeventh      = 10,
    MajorSeventh      = 11,
    Octave            = 12,
};

const char* interval_name(Interval i);
// Invert an interval within an octave (complement to 12).
Interval invert(Interval i);

// ── Scale types ───────────────────────────────────────────────────────────────

enum class ScaleType {
    Major,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,        // ascending form
    PentatonicMajor,
    PentatonicMinor,
    Blues,
    WholeTone,
    Diminished,          // alternating whole-half
    Chromatic,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
};

const char* scale_type_name(ScaleType st);

// A Scale is a root pitch class + type.
struct Scale {
    PitchClass root = PitchClass::C;
    ScaleType  type = ScaleType::Major;

    // Returns the semitone intervals that make up this scale (from root).
    std::vector<int> intervals() const;

    // Returns all MIDI notes in this scale within [midi_low, midi_high].
    std::vector<int> notes_in_range(int midi_low = 48, int midi_high = 84) const;

    // Returns the degree-th scale degree as a MIDI note relative to a root MIDI.
    int degree_to_midi(int root_midi, int degree) const;

    // Returns true if midi_note belongs to this scale (ignoring octave).
    bool contains(int midi_note) const;

    // Nearest scale note to a given MIDI note (snap-to-scale).
    int nearest(int midi_note) const;
};

// ── Chord types ───────────────────────────────────────────────────────────────

enum class ChordType {
    Major,
    Minor,
    Dominant7,
    Major7,
    Minor7,
    Diminished,
    HalfDiminished,
    Augmented,
    Sus2,
    Sus4,
    Add9,
};

const char* chord_type_name(ChordType ct);

struct Chord {
    PitchClass root = PitchClass::C;
    ChordType  type = ChordType::Major;

    // Returns the semitone offsets from the root (e.g. {0, 4, 7} for major).
    std::vector<int> intervals() const;

    // Returns MIDI notes for this chord voiced starting at root_midi.
    std::vector<int> voicing(int root_midi) const;

    // Returns a chord built on the given scale degree (1-indexed).
    static Chord diatonic(const Scale& scale, int degree);
};

// ── Chord progression ─────────────────────────────────────────────────────────

struct ChordEvent {
    Chord        chord;
    int          root_midi = 60;
    float        duration_beats = 1.f;
    float        velocity       = 0.8f;
};

// Build a common progression from Roman-numeral degree list (1-indexed).
std::vector<ChordEvent> build_progression(const Scale& scale, int root_midi,
                                          const std::vector<int>& degrees,
                                          float beats_per_chord = 4.f);

} // namespace music_theory
} // namespace trekker

#endif // TREKKER_MUSIC_ASSETS_MUSIC_THEORY_H
