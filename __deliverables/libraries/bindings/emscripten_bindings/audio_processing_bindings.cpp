// Emscripten bindings for trekker::audio (audio_processing)
// Mirrors the C++ API from:
//   _deliverables/libraries/groups/audio_visual_group/VIDEO_ASSETS/audio_processing/headers/audio_processing.h
//
// Build (from repo root, with emsdk activated):
//   cmake --build build --config Release --target audio_processing_js
//
// Usage in browser (MODULARIZE=1):
//   const mod = await createAudioProcessingModule();
//   const buf = mod.AudioBuffer.silent(44100, 1, 44100);
//   const filtered = mod.low_pass_filter(buf, 800.0);

#include <emscripten/bind.h>
#include "audio_processing.h"

using namespace emscripten;
using namespace trekker::audio;

// ── SampleFormat ─────────────────────────────────────────────────────────────

static int sample_format_S16() { return static_cast<int>(SampleFormat::S16); }
static int sample_format_S32() { return static_cast<int>(SampleFormat::S32); }
static int sample_format_F32() { return static_cast<int>(SampleFormat::F32); }
static int sample_format_F64() { return static_cast<int>(SampleFormat::F64); }

// ── AudioBuffer wrapper ───────────────────────────────────────────────────────
// Emscripten cannot bind std::vector<std::vector<float>> directly, so we
// expose helper methods to read/write individual channels as flat vectors.

struct AudioBufferWrapper {
    AudioBuffer buf;

    AudioBufferWrapper() = default;
    explicit AudioBufferWrapper(AudioBuffer b) : buf(std::move(b)) {}

    // Factory mirrors AudioBuffer::silent
    static AudioBufferWrapper make_silent(uint32_t sample_rate, uint32_t channels,
                                          size_t num_samples) {
        return AudioBufferWrapper(AudioBuffer::silent(sample_rate, channels, num_samples));
    }

    uint32_t get_sample_rate()  const { return buf.sample_rate; }
    uint32_t get_num_channels() const { return buf.num_channels; }
    size_t   get_num_samples()  const { return buf.num_samples(); }
    double   duration_us()      const { return static_cast<double>(buf.duration_us()); }

    // Read channel ch as a flat float vector
    std::vector<float> get_channel(uint32_t ch) const {
        if (ch >= buf.planes.size()) return {};
        return buf.planes[ch];
    }

    // Write a flat float vector into channel ch
    void set_channel(uint32_t ch, const std::vector<float>& samples) {
        while (buf.planes.size() <= ch) buf.planes.emplace_back();
        buf.planes[ch] = samples;
        buf.num_channels = static_cast<uint32_t>(buf.planes.size());
        // Resize other channels to match
        size_t n = samples.size();
        for (auto& plane : buf.planes) {
            if (plane.size() != n) plane.resize(n, 0.0f);
        }
    }

    AudioBufferWrapper clone() const { return AudioBufferWrapper(buf.clone()); }
};

// ── Free-function wrappers ────────────────────────────────────────────────────

static AudioBufferWrapper js_mix(const std::vector<AudioBufferWrapper>& wrappers,
                                 const std::vector<float>& gains) {
    std::vector<AudioBuffer> inputs;
    inputs.reserve(wrappers.size());
    for (const auto& w : wrappers) inputs.push_back(w.buf);
    return AudioBufferWrapper(mix(inputs, gains));
}

static AudioBufferWrapper js_peak_normalize(const AudioBufferWrapper& w, float target_peak) {
    return AudioBufferWrapper(peak_normalize(w.buf, target_peak));
}
static AudioBufferWrapper js_rms_normalize(const AudioBufferWrapper& w, float target_rms) {
    return AudioBufferWrapper(rms_normalize(w.buf, target_rms));
}
static AudioBufferWrapper js_apply_gain_envelope(const AudioBufferWrapper& w,
                                                  const std::vector<float>& gains) {
    return AudioBufferWrapper(apply_gain_envelope(w.buf, gains));
}
static AudioBufferWrapper js_fade_in(const AudioBufferWrapper& w, size_t fade_samples) {
    return AudioBufferWrapper(fade_in(w.buf, fade_samples));
}
static AudioBufferWrapper js_fade_out(const AudioBufferWrapper& w, size_t fade_samples) {
    return AudioBufferWrapper(fade_out(w.buf, fade_samples));
}
static AudioBufferWrapper js_low_pass_filter(const AudioBufferWrapper& w, float cutoff_hz) {
    return AudioBufferWrapper(low_pass_filter(w.buf, cutoff_hz));
}
static AudioBufferWrapper js_high_pass_filter(const AudioBufferWrapper& w, float cutoff_hz) {
    return AudioBufferWrapper(high_pass_filter(w.buf, cutoff_hz));
}
static AudioBufferWrapper js_delay_effect(const AudioBufferWrapper& w,
                                           size_t delay_samples, float feedback) {
    return AudioBufferWrapper(delay_effect(w.buf, delay_samples, feedback));
}
static AudioBufferWrapper js_compress(const AudioBufferWrapper& w,
                                       float threshold, float ratio,
                                       float attack_ms, float release_ms,
                                       float makeup_gain) {
    return AudioBufferWrapper(compress(w.buf, threshold, ratio, attack_ms, release_ms, makeup_gain));
}

// ── Resampler wrapper ─────────────────────────────────────────────────────────

struct ResamplerWrapper {
    Resampler r;
    ResamplerWrapper(uint32_t src, uint32_t dst, uint32_t ch) : r(src, dst, ch) {}
    AudioBufferWrapper process(const AudioBufferWrapper& w) {
        return AudioBufferWrapper(r.process(w.buf));
    }
    void reset() { r.reset(); }
};

// ── Bindings ──────────────────────────────────────────────────────────────────

EMSCRIPTEN_BINDINGS(audio_processing_module) {
    register_vector<float>("VectorFloat_Audio");
    register_vector<AudioBufferWrapper>("VectorAudioBuffer");

    // SampleFormat constants (exposed as plain ints)
    function("SampleFormat_S16", &sample_format_S16);
    function("SampleFormat_S32", &sample_format_S32);
    function("SampleFormat_F32", &sample_format_F32);
    function("SampleFormat_F64", &sample_format_F64);

    // AudioBuffer
    class_<AudioBufferWrapper>("AudioBuffer")
        .class_function("silent", &AudioBufferWrapper::make_silent)
        .function("clone",           &AudioBufferWrapper::clone)
        .function("get_sample_rate", &AudioBufferWrapper::get_sample_rate)
        .function("num_channels",    &AudioBufferWrapper::get_num_channels)
        .function("num_samples",     &AudioBufferWrapper::get_num_samples)
        .function("duration_us",     &AudioBufferWrapper::duration_us)
        .function("get_channel",     &AudioBufferWrapper::get_channel)
        .function("set_channel",     &AudioBufferWrapper::set_channel)
    ;

    // Resampler
    class_<ResamplerWrapper>("Resampler")
        .constructor<uint32_t, uint32_t, uint32_t>()
        .function("process", &ResamplerWrapper::process)
        .function("reset",   &ResamplerWrapper::reset)
    ;

    // Free functions
    function("mix",                &js_mix);
    function("peak_normalize",     &js_peak_normalize);
    function("rms_normalize",      &js_rms_normalize);
    function("apply_gain_envelope",&js_apply_gain_envelope);
    function("fade_in",            &js_fade_in);
    function("fade_out",           &js_fade_out);
    function("low_pass_filter",    &js_low_pass_filter);
    function("high_pass_filter",   &js_high_pass_filter);
    function("delay_effect",       &js_delay_effect);
    function("compress",           &js_compress);
}
