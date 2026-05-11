#include "../headers/music_theory.h"

static int passed = 0, failed = 0;
#define EXPECT_TRUE(x) do { if(x) ++passed; else ++failed; } while(0)
#define EXPECT_EQ(a,b)  EXPECT_TRUE((a)==(b))

int main() {
    using namespace trekker::music_theory;

    // Pitch-to-MIDI
    EXPECT_EQ(pitch_to_midi(PitchClass::A, 4), 69);
    EXPECT_EQ(pitch_to_midi(PitchClass::C, 4), 60);

    // Transpose
    EXPECT_EQ(transpose(PitchClass::C, 7), PitchClass::G);
    EXPECT_EQ(transpose(PitchClass::B, 1), PitchClass::C);

    // Scale intervals
    {
        Scale s; s.root = PitchClass::C; s.type = ScaleType::Major;
        const auto ivs = s.intervals();
        EXPECT_EQ(ivs.size(), std::size_t(7));
        EXPECT_EQ(ivs[0], 0);
        EXPECT_EQ(ivs[2], 4); // E
    }

    // Scale contains
    {
        Scale s; s.root = PitchClass::C; s.type = ScaleType::Major;
        EXPECT_TRUE(s.contains(60)); // C4
        EXPECT_TRUE(s.contains(62)); // D4
        EXPECT_TRUE(!s.contains(61)); // C#4 not in C major
    }

    // Scale degree_to_midi
    {
        Scale s; s.root = PitchClass::C; s.type = ScaleType::Major;
        EXPECT_EQ(s.degree_to_midi(60, 1), 60); // root
        EXPECT_EQ(s.degree_to_midi(60, 5), 67); // G
    }

    // Chord voicing
    {
        Chord c; c.root = PitchClass::C; c.type = ChordType::Major;
        const auto v = c.voicing(60);
        EXPECT_EQ(v.size(), std::size_t(3));
        EXPECT_EQ(v[0], 60); EXPECT_EQ(v[1], 64); EXPECT_EQ(v[2], 67);
    }

    // Diatonic chord on scale degree I of C major = C major
    {
        Scale s; s.root = PitchClass::C; s.type = ScaleType::Major;
        const auto chord = Chord::diatonic(s, 1);
        EXPECT_EQ(chord.root, PitchClass::C);
        EXPECT_EQ(chord.type, ChordType::Major);
    }

    // Progression I-IV-V-I in C major
    {
        Scale s; s.root = PitchClass::C; s.type = ScaleType::Major;
        const auto prog = build_progression(s, 60, {1, 4, 5, 1}, 4.f);
        EXPECT_EQ(prog.size(), std::size_t(4));
        EXPECT_EQ(prog[0].chord.type, ChordType::Major); // I
    }

    // Interval inversion
    EXPECT_EQ(invert(Interval::PerfectFifth), Interval::PerfectFourth);

    return failed > 0 ? 1 : 0;
}
