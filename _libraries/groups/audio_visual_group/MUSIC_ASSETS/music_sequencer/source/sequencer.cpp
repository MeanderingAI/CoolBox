#include "music_sequencer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace trekker {
namespace sequencer {

// ── Pattern ───────────────────────────────────────────────────────────────────

Pattern::Pattern(std::size_t n, float bps)
    : steps_(n), beats_per_step_(bps) {}

void Pattern::set_global_velocity(float v) {
    for (auto& s : steps_) if (s.active) s.velocity = v;
}

void Pattern::transpose(int semitones) {
    for (auto& s : steps_)
        if (s.active) s.midi_note = std::max(0, std::min(127, s.midi_note + semitones));
}

void Pattern::quantise_to_scale(const music_theory::Scale& scale) {
    for (auto& s : steps_)
        if (s.active) s.midi_note = scale.nearest(s.midi_note);
}

void Pattern::set_num_steps(std::size_t n) {
    steps_.resize(n);
}

// ── MixerBus ──────────────────────────────────────────────────────────────────

MixerBus::MixerBus(std::size_t n) : tracks_(n) {}

TrackParams& MixerBus::track(std::size_t i) { return tracks_.at(i); }
const TrackParams& MixerBus::track(std::size_t i) const { return tracks_.at(i); }

void  MixerBus::set_master_volume(float v) { std::lock_guard<std::mutex> lk(mtx_); master_volume_ = v; }
float MixerBus::master_volume() const { std::lock_guard<std::mutex> lk(mtx_); return master_volume_; }
void  MixerBus::set_bpm(float b) { std::lock_guard<std::mutex> lk(mtx_); bpm_ = b; }
float MixerBus::bpm() const { std::lock_guard<std::mutex> lk(mtx_); return bpm_; }

// ── Sequencer ─────────────────────────────────────────────────────────────────

Sequencer::Sequencer(Config cfg)
    : cfg_(cfg)
    , bus_(cfg.num_tracks)
    , bpm_(cfg.bpm)
{
    patterns_.reserve(cfg.num_tracks);
    for (std::size_t i = 0; i < cfg.num_tracks; ++i)
        patterns_.emplace_back(cfg.steps_per_track, cfg.beats_per_step);
    current_step_.assign(cfg.num_tracks, 0);
    active_notes_.resize(cfg.num_tracks, nullptr);
}

float Sequencer::samples_per_step() const {
    const float beats_per_second = bpm_.load() / 60.f;
    const float step_duration_s  = cfg_.beats_per_step / beats_per_second;
    return step_duration_s * static_cast<float>(cfg_.sample_rate);
}

// ── Slider controls ───────────────────────────────────────────────────────────

void Sequencer::set_bpm(float b) {
    bpm_.store(std::max(1.f, std::min(300.f, b)));
    bus_.set_bpm(bpm_.load());
}
void Sequencer::set_master_volume(float v)            { bus_.set_master_volume(v); }
void Sequencer::set_track_volume(std::size_t t, float v)   { bus_.track(t).volume = v; }
void Sequencer::set_track_pan(std::size_t t, float p)      { bus_.track(t).pan = p; }
void Sequencer::set_track_mute(std::size_t t, bool m)      { bus_.track(t).muted = m; }
void Sequencer::set_track_solo(std::size_t t, bool s)      { bus_.track(t).soloed = s; }
void Sequencer::set_track_velocity(std::size_t t, float v) {
    patterns_[t].set_global_velocity(v);
}
void Sequencer::set_step_active(std::size_t t, std::size_t s, bool a) {
    patterns_[t].step(s).active = a;
}
void Sequencer::set_step_note(std::size_t t, std::size_t s, int n) {
    patterns_[t].step(s).midi_note = n;
}
void Sequencer::set_step_velocity(std::size_t t, std::size_t s, float v) {
    patterns_[t].step(s).velocity = v;
}
void Sequencer::set_step_gate(std::size_t t, std::size_t s, float g) {
    patterns_[t].step(s).gate = g;
}
void Sequencer::set_adsr(std::size_t t, const music::AdsrParams& p) {
    bus_.track(t).adsr = p;
    if (active_notes_[t]) active_notes_[t]->set_adsr(p);
}
void Sequencer::set_oscillator(std::size_t t, const music::OscillatorParams& p) {
    bus_.track(t).oscillator = p;
}

// ── Playback ──────────────────────────────────────────────────────────────────

void Sequencer::play()  { playing_.store(true);  }
void Sequencer::pause() { playing_.store(false); }
void Sequencer::stop()  {
    playing_.store(false);
    sample_accumulator_ = 0.0;
    position_beats_     = 0.0;
    std::fill(current_step_.begin(), current_step_.end(), 0);
    for (auto& n : active_notes_) n.reset();
}

std::size_t Sequencer::render(std::vector<float>& stereo_out, std::size_t num_samples) {
    std::lock_guard<std::mutex> lk(render_mtx_);
    stereo_out.assign(num_samples * 2, 0.f);
    if (!playing_.load()) return num_samples;

    const float sps = samples_per_step();
    const float master = bus_.master_volume();

    // Determine if any track is soloed.
    bool any_solo = false;
    for (std::size_t t = 0; t < cfg_.num_tracks; ++t)
        if (bus_.track(t).soloed) { any_solo = true; break; }

    std::vector<float> mono_buf;

    for (std::size_t i = 0; i < num_samples; ++i) {
        // Advance steps as needed.
        sample_accumulator_ += 1.0;
        while (sample_accumulator_ >= sps) {
            sample_accumulator_ -= sps;
            // Trigger next step on each track.
            for (std::size_t t = 0; t < cfg_.num_tracks; ++t) {
                auto& pat = patterns_[t];
                const std::size_t cur = current_step_[t];
                const auto& ev = pat.step(cur);
                if (ev.active) {
                    // Probability gate
                    const float r = static_cast<float>(std::rand()) / RAND_MAX;
                    if (r <= ev.probability) {
                        music::NoteParams np;
                        np.midi_note   = ev.midi_note;
                        np.sample_rate = cfg_.sample_rate;
                        np.velocity    = ev.velocity;
                        np.adsr        = bus_.track(t).adsr;
                        np.oscillators = { bus_.track(t).oscillator };
                        auto gen = std::make_unique<music::NoteGenerator>(np);
                        gen->note_on();
                        active_notes_[t] = std::move(gen);
                        if (step_cb_) step_cb_(t, cur, ev);
                    }
                }
                current_step_[t] = (cur + 1) % pat.num_steps();
            }
            position_beats_ += cfg_.beats_per_step;
        }

        // Render one sample from each active note.
        for (std::size_t t = 0; t < cfg_.num_tracks; ++t) {
            auto& note = active_notes_[t];
            if (!note || note->is_done()) continue;
            const auto& tp = bus_.track(t);
            if (tp.muted) continue;
            if (any_solo && !tp.soloed) continue;

            mono_buf.assign(1, 0.f);
            note->generate(mono_buf, 1);
            const float s = mono_buf[0] * tp.volume * master;
            // Pan: left = cos(θ), right = sin(θ) where θ = (pan+1)/2 * π/2
            const float theta = (tp.pan + 1.f) * 0.5f * 1.5707963f;
            stereo_out[i * 2    ] += s * std::cos(theta);
            stereo_out[i * 2 + 1] += s * std::sin(theta);
        }

        // Hard clip master output
        stereo_out[i * 2    ] = std::max(-1.f, std::min(1.f, stereo_out[i * 2    ]));
        stereo_out[i * 2 + 1] = std::max(-1.f, std::min(1.f, stereo_out[i * 2 + 1]));
    }

    return num_samples;
}

} // namespace sequencer
} // namespace trekker
