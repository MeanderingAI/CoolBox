#ifndef COOLBOX_APP_ASSETS_AUDIO_MIXER_HPP
#define COOLBOX_APP_ASSETS_AUDIO_MIXER_HPP

#include <algorithm>
#include <memory>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace app_assets {
namespace audio_mixer {

// ── Slider / knob parameter identifiers ──────────────────────────────────────

enum class ChannelParam {
    Volume,       // fader  [0.0, 1.0]
    Pan,          // knob   [-1.0, 1.0]
    GainDb,       // trim   [-24.0, +24.0] dB
    SendLevel,    // aux send level [0.0, 1.0]
};

// ── VU meter ──────────────────────────────────────────────────────────────────
// Tracks peak and RMS for a stereo channel pair.

struct VuMeterState {
    float peak_l    = 0.f;  // linear [0, 1]
    float peak_r    = 0.f;
    float rms_l     = 0.f;
    float rms_r     = 0.f;
    bool  clipping  = false; // peak > 0.99
};

class VuMeter {
    public:
        VuMeter() = default;
    VuMeter(const VuMeter&) = delete;
    VuMeter& operator=(const VuMeter&) = delete;
public:
    // Call after each render block with interleaved stereo samples.
    void update(const std::vector<float>& stereo_samples);

    // Call once per display frame to apply peak-hold decay.
    void decay(float decay_factor = 0.95f);

    void reset();

    VuMeterState state() const;

private:
    mutable std::mutex mtx_;
    float peak_l_  = 0.f;
    float peak_r_  = 0.f;
    float sq_sum_l_= 0.f;
    float sq_sum_r_= 0.f;
    std::size_t sample_count_ = 0;
};

// ── Channel strip state ───────────────────────────────────────────────────────

struct ChannelStripState {
    std::string name      = "Ch";
    float volume          = 0.8f;    // fader  [0.0, 1.0]
    float pan             = 0.0f;    // knob   [-1.0, 1.0]
    float gain_db         = 0.0f;    // pre-fader gain trim
    float send_level      = 0.0f;    // aux send
    bool  muted           = false;
    bool  soloed          = false;
    bool  pre_fader_listen= false;   // PFL
    int   midi_channel    = -1;      // -1 = not bound
};

// ── Channel strip ─────────────────────────────────────────────────────────────
// Owns a VuMeter; applies gain, fader, and pan to a mono or stereo audio block.

class ChannelStrip {
    ChannelStrip(const ChannelStrip&) = delete;
    ChannelStrip& operator=(const ChannelStrip&) = delete;
public:
    explicit ChannelStrip(std::size_t id, ChannelStripState state = {});

    std::size_t               id()       const { return id_; }
    const ChannelStripState&  state()    const { return state_; }
    VuMeterState              vu()       const { return vu_.state(); }

    // Slider setters (can be called from the GUI thread).
    void set_volume(float v)    { state_.volume   = std::max(0.f, std::min(1.f, v)); }
    void set_pan(float p)       { state_.pan      = std::max(-1.f, std::min(1.f, p)); }
    void set_gain_db(float db)  { state_.gain_db  = std::max(-24.f, std::min(24.f, db)); }
    void set_send_level(float l){ state_.send_level = std::max(0.f, std::min(1.f, l)); }
    void set_mute(bool m)       { state_.muted    = m; }
    void set_solo(bool s)       { state_.soloed   = s; }
    void set_name(std::string n){ state_.name = std::move(n); }
    void set_state(ChannelStripState s) { state_ = std::move(s); }

    // Process mono input into stereo output (interleaved L,R).
    // Applies gain, fader, and pan law.  Mute zeroes the output.
    void process(const std::vector<float>& mono_in,
                 std::vector<float>&       stereo_out,
                 bool                      any_solo_active);

    // Decay VU peak hold.
    void decay_vu(float factor = 0.95f) { vu_.decay(factor); }

private:
    std::size_t      id_;
    ChannelStripState state_;
    VuMeter          vu_;
};

// ── Master section ────────────────────────────────────────────────────────────

struct MasterSectionState {
    float volume           = 1.0f;    // master fader [0.0, 1.2]
    float pan              = 0.0f;    // master balance [-1.0, 1.0]
    float limiter_threshold= 0.99f;   // brick-wall limiter ceiling [0.5, 1.0]
    bool  limiter_enabled  = true;
};

// ── Mixer console ─────────────────────────────────────────────────────────────
// Owns N ChannelStrips + a master section.
// GUI binds to slider events via a callback.

class MixerConsole {
public:
    // Callback fired whenever any parameter changes: (channel_id, param, value).
    // channel_id == SIZE_MAX → master section event.
    using ParamCallback = std::function<void(std::size_t, ChannelParam, float)>;

    explicit MixerConsole(std::size_t num_channels = 8,
                          std::uint32_t sample_rate = 48000);

    // ── Channel management ────────────────────────────────────────────────
    std::size_t add_channel(const std::string& name = "");
    void        remove_channel(std::size_t id);
    ChannelStrip*       channel(std::size_t id);
    const ChannelStrip* channel(std::size_t id) const;
    std::size_t         num_channels() const { return channels_.size(); }

    // ── Slider controls (GUI thread → audio thread safe) ──────────────────
    void set_channel_volume(std::size_t id, float v);
    void set_channel_pan(std::size_t id, float p);
    void set_channel_gain_db(std::size_t id, float db);
    void set_channel_send(std::size_t id, float level);
    void set_channel_mute(std::size_t id, bool m);
    void set_channel_solo(std::size_t id, bool s);
    void set_master_volume(float v);
    void set_master_pan(float p);
    void set_limiter_threshold(float t);
    void set_limiter_enabled(bool e);

    MasterSectionState master_state() const { return master_; }

    // ── Callback binding ──────────────────────────────────────────────────
    void set_param_callback(ParamCallback cb) { param_cb_ = std::move(cb); }

    // ── Audio processing ──────────────────────────────────────────────────
    // Supply mono audio for each channel (indexed by insertion order).
    // Returns stereo (interleaved L,R) mix of all channels through master.
    std::vector<float> mix(const std::vector<std::vector<float>>& channel_inputs,
                           std::size_t num_samples);

    // VU access (for display refresh).
    VuMeterState channel_vu(std::size_t id) const;
    VuMeterState master_vu() const { return master_vu_.state(); }
    void         decay_all_vu(float factor = 0.95f);

private:
    std::uint32_t              sample_rate_;
    std::vector<std::unique_ptr<ChannelStrip>> channels_;
    std::size_t                next_id_ = 0;
    MasterSectionState         master_;
    VuMeter                    master_vu_;
    ParamCallback              param_cb_;
    mutable std::mutex         mtx_;

    void fire_callback(std::size_t id, ChannelParam p, float v);
};

} // namespace audio_mixer
} // namespace app_assets

#endif // COOLBOX_APP_ASSETS_AUDIO_MIXER_HPP
