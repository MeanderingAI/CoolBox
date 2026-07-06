#include "theory_view.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace music_studio {

static const char* kPitchNames[] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

static const char* kScaleNames[] = {
    "Major","Minor","Dorian","Phrygian","Lydian","Mixolydian",
    "Locrian","Harmonic Minor","Melodic Minor","Pentatonic Major",
    "Pentatonic Minor","Blues","Whole Tone","Diminished","Chromatic"
};

// Semitone intervals from root for each scale (first 10; rest abridged).
static const std::vector<int> kScaleIntervals[] = {
    {0,2,4,5,7,9,11},          // Major
    {0,2,3,5,7,8,10},          // Minor
    {0,2,3,5,7,9,10},          // Dorian
    {0,1,3,5,7,8,10},          // Phrygian
    {0,2,4,6,7,9,11},          // Lydian
    {0,2,4,5,7,9,10},          // Mixolydian
    {0,1,3,5,6,8,10},          // Locrian
    {0,2,3,5,7,8,11},          // Harmonic Minor
    {0,2,3,5,7,9,11},          // Melodic Minor
    {0,2,4,7,9},               // Pentatonic Major
    {0,3,5,7,10},              // Pentatonic Minor
    {0,3,5,6,7,10},            // Blues
    {0,2,4,6,8,10},            // Whole Tone
    {0,2,3,5,6,8,9,11},        // Diminished
    {0,1,2,3,4,5,6,7,8,9,10,11} // Chromatic
};

static const std::vector<int> kChordIntervals[] = {
    {0,4,7},            // Major
    {0,3,7},            // Minor
    {0,3,6},            // Diminished
    {0,4,8},            // Augmented
    {0,5,7},            // Sus4
    {0,2,7},            // Sus2
    {0,4,7,11},         // Major 7
    {0,3,7,10},         // Minor 7
    {0,4,7,10},         // Dom 7
    {0,3,6,9},          // Dim 7
    {0,4,8,10},         // Aug 7
};

static const char* kChordNames[] = {
    "Major","Minor","Diminished","Augmented","Sus4","Sus2",
    "Major7","Minor7","Dom7","Dim7","Aug7"
};

struct TheoryView::Impl {
    int root       = 0;  // C
    int scale_type = 0;  // Major
    int chord_type = 0;  // Major
    int transpose  = 0;
    std::vector<std::pair<int,int>> progression;
};

TheoryView::TheoryView() : impl_(new Impl) {}
TheoryView::~TheoryView() { delete impl_; }

void TheoryView::init() {
    std::cout << "[TheoryView] init — root=" << kPitchNames[impl_->root]
              << " scale=" << kScaleNames[impl_->scale_type] << "\n";
}

void TheoryView::tick(std::int64_t /*delta_us*/) {}

void TheoryView::set_root(int pc) {
    impl_->root = ((pc % 12) + 12) % 12;
}
void TheoryView::set_scale_type(int st) {
    impl_->scale_type = std::max(0, std::min(14, st));
}
int TheoryView::root()       const { return impl_->root;       }
int TheoryView::scale_type() const { return impl_->scale_type; }

std::vector<int> TheoryView::scale_notes() const {
    const auto& iv = kScaleIntervals[impl_->scale_type];
    std::vector<int> notes;
    notes.reserve(iv.size());
    for (int i : iv) notes.push_back((impl_->root + i) % 12);
    return notes;
}

void TheoryView::set_chord_type(int ct) {
    impl_->chord_type = std::max(0, std::min(10, ct));
}
int TheoryView::chord_type() const { return impl_->chord_type; }

std::vector<int> TheoryView::chord_intervals() const {
    return kChordIntervals[impl_->chord_type];
}

void TheoryView::add_chord_to_progression(int root_offset, int ct) {
    if (impl_->progression.size() < 8)
        impl_->progression.emplace_back(root_offset, ct);
}
void TheoryView::clear_progression() { impl_->progression.clear(); }
const std::vector<std::pair<int,int>>& TheoryView::progression() const {
    return impl_->progression;
}

void TheoryView::set_transpose(int s) { impl_->transpose = s; }
int  TheoryView::transpose()    const { return impl_->transpose; }

std::string TheoryView::current_scale_name() const {
    std::string s = kPitchNames[impl_->root];
    s += ' ';
    s += kScaleNames[impl_->scale_type];
    return s;
}

} // namespace music_studio
