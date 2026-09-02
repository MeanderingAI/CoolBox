#include "sequencer_view.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

// Conditionally include sequencer when available.
#if __has_include(<music_sequencer.hpp>)
#  include <music_sequencer.hpp>
#  define MS_HAS_SEQUENCER 1
#endif

namespace music_studio {

struct SequencerView::Impl {
    double bpm          = 120.0;
    bool   playing      = false;

    struct Step {
        bool  active    = false;
        int   note      = 60;   // middle C
        float velocity  = 0.8f;
        float gate      = 0.5f;
    };
    std::array<std::array<Step, kMaxSteps>, kMaxTracks> grid;

    struct TrackParams {
        float vol      = 1.0f;
        float pan      = 0.0f;
        bool  muted    = false;
        int   waveform = 0;     // Sine
        float attack   = 0.01f;
        float decay    = 0.1f;
        float sustain  = 0.7f;
        float release  = 0.3f;
    };
    std::array<TrackParams, kMaxTracks> tracks;
};

SequencerView::SequencerView() : impl_(new Impl) {}
SequencerView::~SequencerView() { delete impl_; }

void SequencerView::init() {
    std::cout << "[SequencerView] init — " << kMaxTracks << " tracks x "
              << kMaxSteps << " steps\n";
}

void SequencerView::tick(std::int64_t /*delta_us*/) {
    // Real implementation: advance playback position, render audio frames,
    // send to mixer bus, and trigger GUI redraw.
}

void SequencerView::set_bpm(double bpm) {
    impl_->bpm = std::max(40.0, std::min(300.0, bpm));
    std::cout << "[SequencerView] BPM = " << impl_->bpm << "\n";
}

void SequencerView::play()  { impl_->playing = true;  std::cout << "[SequencerView] play\n";  }
void SequencerView::pause() { impl_->playing = false; std::cout << "[SequencerView] pause\n"; }
void SequencerView::stop()  { impl_->playing = false; std::cout << "[SequencerView] stop\n";  }

void SequencerView::toggle_step(int t, int s) {
    if (t < 0 || t >= kMaxTracks || s < 0 || s >= kMaxSteps) return;
    impl_->grid[t][s].active = !impl_->grid[t][s].active;
}

void SequencerView::set_step_note(int t, int s, int n) {
    if (t < 0 || t >= kMaxTracks || s < 0 || s >= kMaxSteps) return;
    impl_->grid[t][s].note = std::max(0, std::min(127, n));
}

void SequencerView::set_step_velocity(int t, int s, float v) {
    if (t < 0 || t >= kMaxTracks || s < 0 || s >= kMaxSteps) return;
    impl_->grid[t][s].velocity = std::max(0.f, std::min(1.f, v));
}

void SequencerView::set_step_gate(int t, int s, float g) {
    if (t < 0 || t >= kMaxTracks || s < 0 || s >= kMaxSteps) return;
    impl_->grid[t][s].gate = std::max(0.f, std::min(1.f, g));
}

void SequencerView::set_track_volume(int t, float v) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].vol = std::max(0.f, std::min(1.f, v));
}

void SequencerView::set_track_pan(int t, float p) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].pan = std::max(-1.f, std::min(1.f, p));
}

void SequencerView::set_track_muted(int t, bool m) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].muted = m;
}

void SequencerView::set_track_waveform(int t, int w) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].waveform = w;
}

void SequencerView::set_track_attack(int t, float s) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].attack = std::max(0.f, s);
}
void SequencerView::set_track_decay(int t, float s) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].decay = std::max(0.f, s);
}
void SequencerView::set_track_sustain(int t, float l) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].sustain = std::max(0.f, std::min(1.f, l));
}
void SequencerView::set_track_release(int t, float s) {
    if (t < 0 || t >= kMaxTracks) return;
    impl_->tracks[t].release = std::max(0.f, s);
}

double SequencerView::bpm()        const { return impl_->bpm;     }
bool   SequencerView::is_playing() const { return impl_->playing; }

} // namespace music_studio
