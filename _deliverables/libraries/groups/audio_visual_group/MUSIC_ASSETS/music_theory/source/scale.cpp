#include "music_theory.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trekker {
namespace music_theory {

// ── PitchClass ────────────────────────────────────────────────────────────────

const char* pitch_class_name(PitchClass pc) {
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const int idx = static_cast<int>(pc);
    return (idx >= 0 && idx < 12) ? names[idx] : "?";
}

PitchClass transpose(PitchClass pc, int semitones) {
    return static_cast<PitchClass>(((static_cast<int>(pc) + semitones) % 12 + 12) % 12);
}

// ── Interval ──────────────────────────────────────────────────────────────────

const char* interval_name(Interval i) {
    static const char* names[] = {
        "Unison","Minor 2nd","Major 2nd","Minor 3rd","Major 3rd",
        "Perfect 4th","Tritone","Perfect 5th","Minor 6th","Major 6th",
        "Minor 7th","Major 7th","Octave"
    };
    const int idx = static_cast<int>(i);
    return (idx >= 0 && idx <= 12) ? names[idx] : "?";
}

Interval invert(Interval i) {
    const int v = static_cast<int>(i);
    return (v == 0) ? Interval::Octave : static_cast<Interval>(12 - v);
}

// ── Scale ─────────────────────────────────────────────────────────────────────

const char* scale_type_name(ScaleType st) {
    switch (st) {
        case ScaleType::Major:          return "Major";
        case ScaleType::NaturalMinor:   return "Natural Minor";
        case ScaleType::HarmonicMinor:  return "Harmonic Minor";
        case ScaleType::MelodicMinor:   return "Melodic Minor";
        case ScaleType::PentatonicMajor:return "Pentatonic Major";
        case ScaleType::PentatonicMinor:return "Pentatonic Minor";
        case ScaleType::Blues:          return "Blues";
        case ScaleType::WholeTone:      return "Whole Tone";
        case ScaleType::Diminished:     return "Diminished";
        case ScaleType::Chromatic:      return "Chromatic";
        case ScaleType::Dorian:         return "Dorian";
        case ScaleType::Phrygian:       return "Phrygian";
        case ScaleType::Lydian:         return "Lydian";
        case ScaleType::Mixolydian:     return "Mixolydian";
        case ScaleType::Locrian:        return "Locrian";
    }
    return "Unknown";
}

std::vector<int> Scale::intervals() const {
    switch (type) {
        case ScaleType::Major:           return {0,2,4,5,7,9,11};
        case ScaleType::NaturalMinor:    return {0,2,3,5,7,8,10};
        case ScaleType::HarmonicMinor:   return {0,2,3,5,7,8,11};
        case ScaleType::MelodicMinor:    return {0,2,3,5,7,9,11};
        case ScaleType::PentatonicMajor: return {0,2,4,7,9};
        case ScaleType::PentatonicMinor: return {0,3,5,7,10};
        case ScaleType::Blues:           return {0,3,5,6,7,10};
        case ScaleType::WholeTone:       return {0,2,4,6,8,10};
        case ScaleType::Diminished:      return {0,2,3,5,6,8,9,11};
        case ScaleType::Chromatic:       return {0,1,2,3,4,5,6,7,8,9,10,11};
        case ScaleType::Dorian:          return {0,2,3,5,7,9,10};
        case ScaleType::Phrygian:        return {0,1,3,5,7,8,10};
        case ScaleType::Lydian:          return {0,2,4,6,7,9,11};
        case ScaleType::Mixolydian:      return {0,2,4,5,7,9,10};
        case ScaleType::Locrian:         return {0,1,3,5,6,8,10};
    }
    return {0};
}

std::vector<int> Scale::notes_in_range(int lo, int hi) const {
    const int root_pc = static_cast<int>(root);
    const auto ivs = intervals();
    std::vector<int> result;
    for (int midi = lo; midi <= hi; ++midi)
        for (int iv : ivs)
            if (((midi - root_pc - iv) % 12 + 12) % 12 == 0)
                result.push_back(midi);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

int Scale::degree_to_midi(int root_midi, int degree) const {
    const auto ivs = intervals();
    if (degree < 1) return root_midi;
    const int idx   = (degree - 1) % static_cast<int>(ivs.size());
    const int oct   = (degree - 1) / static_cast<int>(ivs.size());
    return root_midi + ivs[idx] + 12 * oct;
}

bool Scale::contains(int midi_note) const {
    const int pc = ((midi_note - static_cast<int>(root)) % 12 + 12) % 12;
    const auto ivs = intervals();
    return std::find(ivs.begin(), ivs.end(), pc) != ivs.end();
}

int Scale::nearest(int midi_note) const {
    const auto notes = notes_in_range(std::max(0, midi_note - 12),
                                      std::min(127, midi_note + 12));
    if (notes.empty()) return midi_note;
    return *std::min_element(notes.begin(), notes.end(),
        [midi_note](int a, int b) {
            return std::abs(a - midi_note) < std::abs(b - midi_note);
        });
}

// ── Chord ─────────────────────────────────────────────────────────────────────

const char* chord_type_name(ChordType ct) {
    switch (ct) {
        case ChordType::Major:          return "Major";
        case ChordType::Minor:          return "Minor";
        case ChordType::Dominant7:      return "Dominant 7th";
        case ChordType::Major7:         return "Major 7th";
        case ChordType::Minor7:         return "Minor 7th";
        case ChordType::Diminished:     return "Diminished";
        case ChordType::HalfDiminished: return "Half-Diminished";
        case ChordType::Augmented:      return "Augmented";
        case ChordType::Sus2:           return "Sus2";
        case ChordType::Sus4:           return "Sus4";
        case ChordType::Add9:           return "Add9";
    }
    return "?";
}

std::vector<int> Chord::intervals() const {
    switch (type) {
        case ChordType::Major:          return {0, 4, 7};
        case ChordType::Minor:          return {0, 3, 7};
        case ChordType::Dominant7:      return {0, 4, 7, 10};
        case ChordType::Major7:         return {0, 4, 7, 11};
        case ChordType::Minor7:         return {0, 3, 7, 10};
        case ChordType::Diminished:     return {0, 3, 6};
        case ChordType::HalfDiminished: return {0, 3, 6, 10};
        case ChordType::Augmented:      return {0, 4, 8};
        case ChordType::Sus2:           return {0, 2, 7};
        case ChordType::Sus4:           return {0, 5, 7};
        case ChordType::Add9:           return {0, 4, 7, 14};
    }
    return {0};
}

std::vector<int> Chord::voicing(int root_midi) const {
    std::vector<int> v;
    for (int iv : intervals()) v.push_back(root_midi + iv);
    return v;
}

Chord Chord::diatonic(const Scale& scale, int degree) {
    // Build a triad stacked in thirds from the given scale degree.
    static const ChordType major_diatonic[] = {
        ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major,
        ChordType::Major, ChordType::Minor, ChordType::Diminished
    };
    static const ChordType minor_diatonic[] = {
        ChordType::Minor, ChordType::Diminished, ChordType::Major, ChordType::Minor,
        ChordType::Minor, ChordType::Major, ChordType::Major
    };
    const bool is_minor = (scale.type == ScaleType::NaturalMinor ||
                           scale.type == ScaleType::HarmonicMinor ||
                           scale.type == ScaleType::MelodicMinor);
    const auto* table = is_minor ? minor_diatonic : major_diatonic;
    const int   idx   = ((degree - 1) % 7 + 7) % 7;
    const int   root_pc = (static_cast<int>(scale.root) + scale.intervals()[idx]) % 12;
    return Chord{ static_cast<PitchClass>(root_pc), table[idx] };
}

// ── Progression ───────────────────────────────────────────────────────────────

std::vector<ChordEvent> build_progression(const Scale& scale, int root_midi,
                                          const std::vector<int>& degrees,
                                          float beats_per_chord) {
    std::vector<ChordEvent> events;
    for (int deg : degrees) {
        ChordEvent ev;
        ev.chord         = Chord::diatonic(scale, deg);
        ev.root_midi     = scale.degree_to_midi(root_midi, deg);
        ev.duration_beats= beats_per_chord;
        ev.velocity      = 0.8f;
        events.push_back(ev);
    }
    return events;
}

} // namespace music_theory
} // namespace trekker
