#include "mixer_view.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace music_studio {

struct ChannelState {
    std::string name;
    float       volume     = 1.f;
    float       pan        = 0.f;
    float       gain_db    = 0.f;
    float       send_level = 0.f;
    bool        muted      = false;
    bool        soloed     = false;
    bool        pfl        = false;
    // VU (populated by mix engine when linked)
    float       peak_l = 0.f, peak_r = 0.f;
    float       rms_l  = 0.f, rms_r  = 0.f;
    bool        clipping = false;
};

struct MixerView::Impl {
    std::vector<ChannelState> channels;
    float master_volume    = 1.f;
    float master_pan       = 0.f;
    float limiter_threshold = -0.3f;
    bool  limiter_enabled  = true;
    float master_peak_l    = 0.f;
    float master_peak_r    = 0.f;
};

MixerView::MixerView() : impl_(new Impl) {}
MixerView::~MixerView()  { delete impl_; }

void MixerView::init() {
    std::cout << "[MixerView] init — max " << kMaxChannels << " channels\n";
}

void MixerView::tick(std::int64_t /*delta_us*/) {
    // Real: decay VU meters, refresh GUI widgets.
}

std::size_t MixerView::add_channel(const std::string& name) {
    if (impl_->channels.size() >= kMaxChannels) {
        std::cerr << "[MixerView] channel limit reached\n";
        return impl_->channels.size() - 1;
    }
    ChannelState ch;
    ch.name = name.empty()
        ? ("Ch " + std::to_string(impl_->channels.size() + 1))
        : name;
    impl_->channels.push_back(ch);
    std::cout << "[MixerView] added channel '" << ch.name << "'\n";
    return impl_->channels.size() - 1;
}

void MixerView::remove_channel(std::size_t id) {
    if (id < impl_->channels.size())
        impl_->channels.erase(impl_->channels.begin() + static_cast<long>(id));
}

void MixerView::set_volume(std::size_t id, float v) {
    if (id < impl_->channels.size())
        impl_->channels[id].volume = std::max(0.f, std::min(1.f, v));
}
void MixerView::set_pan(std::size_t id, float p) {
    if (id < impl_->channels.size())
        impl_->channels[id].pan = std::max(-1.f, std::min(1.f, p));
}
void MixerView::set_gain_db(std::size_t id, float db) {
    if (id < impl_->channels.size())
        impl_->channels[id].gain_db = db;
}
void MixerView::set_send_level(std::size_t id, float v) {
    if (id < impl_->channels.size())
        impl_->channels[id].send_level = std::max(0.f, std::min(1.f, v));
}
void MixerView::set_muted(std::size_t id, bool m) {
    if (id < impl_->channels.size()) impl_->channels[id].muted = m;
}
void MixerView::set_solo(std::size_t id, bool s) {
    if (id < impl_->channels.size()) impl_->channels[id].soloed = s;
}
void MixerView::set_pfl(std::size_t id, bool pfl) {
    if (id < impl_->channels.size()) impl_->channels[id].pfl = pfl;
}
void MixerView::set_name(std::size_t id, const std::string& name) {
    if (id < impl_->channels.size()) impl_->channels[id].name = name;
}

void MixerView::set_master_volume(float v)         { impl_->master_volume    = std::max(0.f, std::min(1.f, v)); }
void MixerView::set_master_pan(float p)            { impl_->master_pan       = std::max(-1.f, std::min(1.f, p)); }
void MixerView::set_limiter_threshold(float db)    { impl_->limiter_threshold = db; }
void MixerView::set_limiter_enabled(bool en)       { impl_->limiter_enabled  = en; }

float MixerView::vu_peak_l(std::size_t id)   const { return id < impl_->channels.size() ? impl_->channels[id].peak_l   : 0.f; }
float MixerView::vu_peak_r(std::size_t id)   const { return id < impl_->channels.size() ? impl_->channels[id].peak_r   : 0.f; }
float MixerView::vu_rms_l(std::size_t id)    const { return id < impl_->channels.size() ? impl_->channels[id].rms_l    : 0.f; }
float MixerView::vu_rms_r(std::size_t id)    const { return id < impl_->channels.size() ? impl_->channels[id].rms_r    : 0.f; }
bool  MixerView::vu_clipping(std::size_t id) const { return id < impl_->channels.size() ? impl_->channels[id].clipping : false; }
float MixerView::master_peak_l()             const { return impl_->master_peak_l; }
float MixerView::master_peak_r()             const { return impl_->master_peak_r; }

} // namespace music_studio
