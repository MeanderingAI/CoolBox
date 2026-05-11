#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace music_studio {

// ── MixerView ─────────────────────────────────────────────────────────────────
// Mixing-console panel backed by audio_mixer_lib (when linked).
//
// Features:
//   • Up to 16 mono channel strips with fader, pan knob, gain, mute, solo
//   • Pre-fader listen (PFL) button per strip
//   • VU meter display (peak + RMS) per strip and for master
//   • Send level slider for an effects bus
//   • Master section: volume, pan, limiter threshold toggle
//   • MIDI channel assignment per strip

class MixerView {
public:
    MixerView();
    ~MixerView();

    void init();
    void tick(std::int64_t delta_us);

    // ── Channel controls ──────────────────────────────────────────────────
    std::size_t add_channel(const std::string& name = "");
    void remove_channel(std::size_t id);

    void set_volume(std::size_t id, float v);      // [0, 1]
    void set_pan(std::size_t id, float pan);       // [-1, +1]
    void set_gain_db(std::size_t id, float db);    // e.g. -12 .. +12
    void set_send_level(std::size_t id, float v);  // [0, 1]
    void set_muted(std::size_t id, bool m);
    void set_solo(std::size_t id, bool s);
    void set_pfl(std::size_t id, bool pfl);
    void set_name(std::size_t id, const std::string& name);

    // ── Master section ────────────────────────────────────────────────────
    void set_master_volume(float v);
    void set_master_pan(float p);
    void set_limiter_threshold(float db);
    void set_limiter_enabled(bool en);

    // ── Metering (normalised 0-1) ─────────────────────────────────────────
    float vu_peak_l(std::size_t id) const;
    float vu_peak_r(std::size_t id) const;
    float vu_rms_l(std::size_t id)  const;
    float vu_rms_r(std::size_t id)  const;
    bool  vu_clipping(std::size_t id) const;

    float master_peak_l() const;
    float master_peak_r() const;

    static constexpr std::size_t kMaxChannels = 16;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace music_studio
