#include "audio_mixer.hpp"
#include <numeric>
#include <stdexcept>

namespace app_assets {
namespace audio_mixer {

// ── VuMeter ───────────────────────────────────────────────────────────────────

void VuMeter::update(const std::vector<float>& stereo) {
    std::lock_guard<std::mutex> lk(mtx_);
    const std::size_t frames = stereo.size() / 2;
    for (std::size_t i = 0; i < frames; ++i) {
        const float l = std::abs(stereo[i * 2    ]);
        const float r = std::abs(stereo[i * 2 + 1]);
        if (l > peak_l_) peak_l_ = l;
        if (r > peak_r_) peak_r_ = r;
        sq_sum_l_ += l * l;
        sq_sum_r_ += r * r;
    }
    sample_count_ += frames;
}

void VuMeter::decay(float factor) {
    std::lock_guard<std::mutex> lk(mtx_);
    peak_l_ *= factor;
    peak_r_ *= factor;
    sq_sum_l_ = 0.f;
    sq_sum_r_ = 0.f;
    sample_count_ = 0;
}

void VuMeter::reset() {
    std::lock_guard<std::mutex> lk(mtx_);
    peak_l_ = peak_r_ = sq_sum_l_ = sq_sum_r_ = 0.f;
    sample_count_ = 0;
}

VuMeterState VuMeter::state() const {
    std::lock_guard<std::mutex> lk(mtx_);
    VuMeterState s;
    s.peak_l   = peak_l_;
    s.peak_r   = peak_r_;
    s.rms_l    = (sample_count_ > 0) ? std::sqrt(sq_sum_l_ / sample_count_) : 0.f;
    s.rms_r    = (sample_count_ > 0) ? std::sqrt(sq_sum_r_ / sample_count_) : 0.f;
    s.clipping = (peak_l_ > 0.99f || peak_r_ > 0.99f);
    return s;
}

// ── ChannelStrip ──────────────────────────────────────────────────────────────

ChannelStrip::ChannelStrip(std::size_t id, ChannelStripState state)
    : id_(id), state_(std::move(state)), vu_() {}

void ChannelStrip::process(const std::vector<float>& mono_in,
                           std::vector<float>&       stereo_out,
                           bool                      any_solo_active) {
    const std::size_t n = mono_in.size();
    stereo_out.assign(n * 2, 0.f);

    if (state_.muted) return;
    if (any_solo_active && !state_.soloed) return;

    // Pre-fader gain (dB → linear).
    const float gain_lin = std::pow(10.f, state_.gain_db / 20.f);
    // Fader.
    const float fader = state_.volume;
    // Pan law: constant-power.
    const float theta = (state_.pan + 1.f) * 0.5f * 1.5707963f;
    const float pan_l = std::cos(theta);
    const float pan_r = std::sin(theta);

    for (std::size_t i = 0; i < n; ++i) {
        const float s = mono_in[i] * gain_lin * fader;
        stereo_out[i * 2    ] = std::max(-1.f, std::min(1.f, s * pan_l));
        stereo_out[i * 2 + 1] = std::max(-1.f, std::min(1.f, s * pan_r));
    }

    vu_.update(stereo_out);
}

// ── MixerConsole ──────────────────────────────────────────────────────────────

MixerConsole::MixerConsole(std::size_t num_channels, std::uint32_t sample_rate)
    : sample_rate_(sample_rate), master_vu_()
{
    for (std::size_t i = 0; i < num_channels; ++i) {
        ChannelStripState s;
        s.name = "Ch " + std::to_string(i + 1);
        channels_.emplace_back(std::make_unique<ChannelStrip>(next_id_++, s));
    }
}

std::size_t MixerConsole::add_channel(const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    ChannelStripState s;
    s.name = name.empty() ? ("Ch " + std::to_string(next_id_ + 1)) : name;
    const std::size_t id = next_id_++;
    channels_.emplace_back(std::make_unique<ChannelStrip>(id, s));
    return id;
}

void MixerConsole::remove_channel(std::size_t id) {
    std::lock_guard<std::mutex> lk(mtx_);
    channels_.erase(std::remove_if(channels_.begin(), channels_.end(),
        [id](const std::unique_ptr<ChannelStrip>& c) { return c->id() == id; }), channels_.end());
}

ChannelStrip* MixerConsole::channel(std::size_t id) {
    for (auto& c : channels_) if (c->id() == id) return c.get();
    return nullptr;
}
const ChannelStrip* MixerConsole::channel(std::size_t id) const {
    for (const auto& c : channels_) if (c->id() == id) return c.get();
    return nullptr;
}

// ── Slider controls ───────────────────────────────────────────────────────────

void MixerConsole::set_channel_volume(std::size_t id, float v) {
    if (auto* c = channel(id)) { c->set_volume(v); fire_callback(id, ChannelParam::Volume, v); }
}
void MixerConsole::set_channel_pan(std::size_t id, float p) {
    if (auto* c = channel(id)) { c->set_pan(p); fire_callback(id, ChannelParam::Pan, p); }
}
void MixerConsole::set_channel_gain_db(std::size_t id, float db) {
    if (auto* c = channel(id)) { c->set_gain_db(db); fire_callback(id, ChannelParam::GainDb, db); }
}
void MixerConsole::set_channel_send(std::size_t id, float level) {
    if (auto* c = channel(id)) { c->set_send_level(level); fire_callback(id, ChannelParam::SendLevel, level); }
}
void MixerConsole::set_channel_mute(std::size_t id, bool m) {
    if (auto* c = channel(id)) c->set_mute(m);
}
void MixerConsole::set_channel_solo(std::size_t id, bool s) {
    if (auto* c = channel(id)) c->set_solo(s);
}
void MixerConsole::set_master_volume(float v) {
    master_.volume = std::max(0.f, std::min(1.2f, v));
    fire_callback(~std::size_t(0), ChannelParam::Volume, v);
}
void MixerConsole::set_master_pan(float p) {
    master_.pan = std::max(-1.f, std::min(1.f, p));
    fire_callback(~std::size_t(0), ChannelParam::Pan, p);
}
void MixerConsole::set_limiter_threshold(float t) {
    master_.limiter_threshold = std::max(0.5f, std::min(1.f, t));
}
void MixerConsole::set_limiter_enabled(bool e) { master_.limiter_enabled = e; }

void MixerConsole::fire_callback(std::size_t id, ChannelParam p, float v) {
    if (param_cb_) param_cb_(id, p, v);
}

// ── Mix ───────────────────────────────────────────────────────────────────────

std::vector<float> MixerConsole::mix(const std::vector<std::vector<float>>& inputs,
                                     std::size_t num_samples) {
    // Determine solo state.
    bool any_solo = false;
    for (const auto& c : channels_) if (c->state().soloed) { any_solo = true; break; }

    std::vector<float> master_buf(num_samples * 2, 0.f);
    std::vector<float> ch_stereo;

    for (std::size_t i = 0; i < channels_.size() && i < inputs.size(); ++i) {
        channels_[i]->process(inputs[i], ch_stereo, any_solo);
        for (std::size_t s = 0; s < master_buf.size(); ++s)
            master_buf[s] += ch_stereo[s];
    }

    // Master fader + pan + limiter.
    const float mvol = master_.volume;
    const float theta = (master_.pan + 1.f) * 0.5f * 1.5707963f;
    const float ml = std::cos(theta) * mvol;
    const float mr = std::sin(theta) * mvol;
    const float limit = master_.limiter_enabled ? master_.limiter_threshold : 1.f;

    for (std::size_t i = 0; i < num_samples; ++i) {
        master_buf[i * 2    ] = std::max(-limit, std::min(limit, master_buf[i * 2    ] * ml));
        master_buf[i * 2 + 1] = std::max(-limit, std::min(limit, master_buf[i * 2 + 1] * mr));
    }

    master_vu_.update(master_buf);
    return master_buf;
}

VuMeterState MixerConsole::channel_vu(std::size_t id) const {
    if (const auto* c = channel(id)) return c->vu();
    return {};
}

void MixerConsole::decay_all_vu(float factor) {
    for (auto& c : channels_) c->decay_vu(factor);
    master_vu_.decay(factor);
}

} // namespace audio_mixer
} // namespace app_assets
