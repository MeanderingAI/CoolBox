#include "note_synthesis.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace trekker {
namespace music {

// ── Note name parser ──────────────────────────────────────────────────────────

int note_name_to_midi(const std::string& name) {
    if (name.size() < 2) return -1;
    const char note_char = static_cast<char>(std::toupper(name[0]));
    const int note_offsets[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    if (note_char < 'A' || note_char > 'G') return -1;
    int base = note_offsets[note_char - 'A'];
    std::size_t pos = 1;
    if (pos < name.size() && (name[pos] == '#' || name[pos] == 'b')) {
        base += (name[pos] == '#') ? 1 : -1;
        ++pos;
    }
    if (pos >= name.size()) return -1;
    try {
        const int octave = std::stoi(name.substr(pos));
        return 12 * (octave + 1) + base;
    } catch (...) { return -1; }
}

// ── AdsrParams ────────────────────────────────────────────────────────────────

AdsrParams AdsrParams::clamped() const {
    AdsrParams r = *this;
    r.attack_s  = std::max(0.001f, r.attack_s);
    r.decay_s   = std::max(0.001f, r.decay_s);
    r.sustain   = std::max(0.0f,   std::min(1.0f, r.sustain));
    r.release_s = std::max(0.001f, r.release_s);
    return r;
}

// ── AdsrEnvelope ─────────────────────────────────────────────────────────────

AdsrEnvelope::AdsrEnvelope(AdsrParams p, std::uint32_t sr)
    : p_(p.clamped()), sample_rate_(sr) {}

void AdsrEnvelope::note_on() {
    stage_ = Stage::Attack;
    level_ = 0.f;
    sample_counter_ = 0;
    stage_start_    = 0;
}

void AdsrEnvelope::note_off() {
    if (stage_ != Stage::Done) {
        stage_       = Stage::Release;
        stage_start_ = sample_counter_;
    }
}

float AdsrEnvelope::next_sample() {
    ++sample_counter_;
    const std::uint64_t elapsed = sample_counter_ - stage_start_;

    switch (stage_) {
        case Stage::Attack: {
            const std::uint64_t atk = static_cast<std::uint64_t>(p_.attack_s * sample_rate_);
            level_ = static_cast<float>(elapsed) / atk;
            if (level_ >= 1.f) { level_ = 1.f; stage_ = Stage::Decay; stage_start_ = sample_counter_; }
            break;
        }
        case Stage::Decay: {
            const std::uint64_t dec = static_cast<std::uint64_t>(p_.decay_s * sample_rate_);
            level_ = 1.f - (1.f - p_.sustain) * static_cast<float>(elapsed) / dec;
            if (level_ <= p_.sustain) { level_ = p_.sustain; stage_ = Stage::Sustain; }
            break;
        }
        case Stage::Sustain:
            level_ = p_.sustain;
            break;
        case Stage::Release: {
            const std::uint64_t rel = static_cast<std::uint64_t>(p_.release_s * sample_rate_);
            level_ = p_.sustain * (1.f - static_cast<float>(elapsed) / rel);
            if (elapsed >= rel) { level_ = 0.f; stage_ = Stage::Done; }
            break;
        }
        case Stage::Done:
        case Stage::Idle:
        default:
            level_ = 0.f;
            break;
    }
    return std::max(0.f, std::min(1.f, level_));
}

void AdsrEnvelope::reset() {
    stage_ = Stage::Idle; level_ = 0.f;
    sample_counter_ = 0; stage_start_ = 0;
}

void AdsrEnvelope::set_attack(float s)   { p_.attack_s  = std::max(0.001f, s); }
void AdsrEnvelope::set_decay(float s)    { p_.decay_s   = std::max(0.001f, s); }
void AdsrEnvelope::set_sustain(float v)  { p_.sustain   = std::max(0.f, std::min(1.f, v)); }
void AdsrEnvelope::set_release(float s)  { p_.release_s = std::max(0.001f, s); }

// ── Oscillator ────────────────────────────────────────────────────────────────

static constexpr double kTwoPi = 6.28318530717958647692;

Oscillator::Oscillator(OscillatorParams p, std::uint32_t sr)
    : p_(p), sample_rate_(sr) { update_increment(); }

void Oscillator::update_increment() {
    const float eff_hz = p_.frequency * std::pow(2.f, p_.detune_cents / 1200.f);
    increment_ = kTwoPi * eff_hz / sample_rate_;
}

float Oscillator::next_sample() {
    float s = 0.f;
    switch (p_.waveform) {
        case Waveform::Sine:
            s = std::sin(static_cast<float>(phase_));
            break;
        case Waveform::Square:
            s = (phase_ < kTwoPi * 0.5) ? 1.f : -1.f;
            break;
        case Waveform::Pulse:
            s = (phase_ < kTwoPi * p_.duty_cycle) ? 1.f : -1.f;
            break;
        case Waveform::Sawtooth:
            s = static_cast<float>(phase_ / kTwoPi) * 2.f - 1.f;
            break;
        case Waveform::Triangle: {
            const float t = static_cast<float>(phase_ / kTwoPi);
            s = (t < 0.5f) ? (4.f * t - 1.f) : (3.f - 4.f * t);
            break;
        }
        case Waveform::Noise:
            noise_seed_ = noise_seed_ * 1664525u + 1013904223u;
            s = static_cast<float>(static_cast<std::int32_t>(noise_seed_)) / 2147483648.f;
            break;
    }
    phase_ += increment_;
    if (phase_ >= kTwoPi) phase_ -= kTwoPi;
    return s * p_.amplitude;
}

void Oscillator::render(std::vector<float>& out, std::size_t count) {
    out.resize(count, 0.f);
    for (std::size_t i = 0; i < count; ++i) out[i] += next_sample();
}

void Oscillator::reset() {
    phase_ = p_.phase_rad; noise_seed_ = 12345; update_increment();
}

// ── NoteGenerator ─────────────────────────────────────────────────────────────

NoteGenerator::NoteGenerator(NoteParams p)
    : params_(std::move(p))
    , envelope_(params_.adsr, params_.sample_rate)
{
    const float hz = midi_to_hz(params_.midi_note);
    for (auto osc_p : params_.oscillators) {
        osc_p.frequency = hz;
        oscillators_.emplace_back(osc_p, params_.sample_rate);
    }
}

void NoteGenerator::note_on()  { envelope_.note_on(); }
void NoteGenerator::note_off() { envelope_.note_off(); }

bool NoteGenerator::generate(std::vector<float>& out, std::size_t count) {
    out.assign(count, 0.f);
    std::vector<float> osc_buf;
    for (auto& osc : oscillators_) {
        osc_buf.assign(count, 0.f);
        osc.render(osc_buf, count);
        for (std::size_t i = 0; i < count; ++i) out[i] += osc_buf[i];
    }
    for (std::size_t i = 0; i < count; ++i) {
        const float env = envelope_.next_sample();
        out[i] = std::max(-1.f, std::min(1.f, out[i] * env * params_.velocity));
    }
    return !envelope_.is_done();
}

void NoteGenerator::set_adsr(const AdsrParams& a) {
    envelope_.set_attack(a.attack_s);
    envelope_.set_decay(a.decay_s);
    envelope_.set_sustain(a.sustain);
    envelope_.set_release(a.release_s);
}

void NoteGenerator::set_oscillator_param(std::size_t idx, const OscillatorParams& p) {
    if (idx >= oscillators_.size()) return;
    oscillators_[idx].set_waveform(p.waveform);
    oscillators_[idx].set_frequency(midi_to_hz(params_.midi_note));
    oscillators_[idx].set_amplitude(p.amplitude);
    oscillators_[idx].set_detune_cents(p.detune_cents);
    oscillators_[idx].set_duty_cycle(p.duty_cycle);
}

} // namespace music
} // namespace trekker
