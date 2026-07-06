#include "audio_processing.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace trekker {
namespace audio {

// ── Format helpers ────────────────────────────────────────────────────────────

int bytes_per_sample(SampleFormat fmt) {
    switch (fmt) {
        case SampleFormat::S16: return 2;
        case SampleFormat::S32: return 4;
        case SampleFormat::F32: return 4;
        case SampleFormat::F64: return 8;
    }
    return 4;
}

const char* sample_format_name(SampleFormat fmt) {
    switch (fmt) {
        case SampleFormat::S16: return "S16";
        case SampleFormat::S32: return "S32";
        case SampleFormat::F32: return "F32";
        case SampleFormat::F64: return "F64";
    }
    return "UNKNOWN";
}

// ── AudioBuffer ───────────────────────────────────────────────────────────────

AudioBuffer AudioBuffer::silent(std::uint32_t rate, std::uint32_t ch,
                                std::size_t n, SampleFormat fmt) {
    AudioBuffer buf;
    buf.sample_rate  = rate;
    buf.num_channels = ch;
    buf.format       = fmt;
    buf.planes.assign(ch, std::vector<float>(n, 0.f));
    return buf;
}

AudioBuffer AudioBuffer::clone() const { return *this; }

// ── Resampler ─────────────────────────────────────────────────────────────────

Resampler::Resampler(std::uint32_t src, std::uint32_t dst, std::uint32_t ch)
    : src_rate_(src), dst_rate_(dst), channels_(ch)
    , prev_samples_(ch, 0.f) {}

void Resampler::reset() {
    std::fill(prev_samples_.begin(), prev_samples_.end(), 0.f);
    phase_ = 0.0;
}

AudioBuffer Resampler::process(const AudioBuffer& in) {
    if (in.num_samples() == 0)
        return AudioBuffer::silent(dst_rate_, channels_, 0);

    const double ratio = static_cast<double>(dst_rate_) / src_rate_;
    const std::size_t out_n = static_cast<std::size_t>(
        std::ceil(in.num_samples() * ratio - phase_));

    AudioBuffer out;
    out.sample_rate  = dst_rate_;
    out.num_channels = channels_;
    out.format       = in.format;
    out.pts          = in.pts;
    out.planes.resize(channels_);

    for (std::uint32_t c = 0; c < channels_; ++c) {
        out.planes[c].reserve(out_n);
        double src_pos = phase_;
        while (src_pos < static_cast<double>(in.num_samples())) {
            const std::size_t idx0 = static_cast<std::size_t>(src_pos);
            const double frac = src_pos - idx0;
            const float s0 = idx0 == 0 ? prev_samples_[c] : in.planes[c][idx0 - 1];
            const float s1 = in.planes[c][idx0];
            out.planes[c].push_back(static_cast<float>(s0 + frac * (s1 - s0)));
            src_pos += 1.0 / ratio;
        }
        prev_samples_[c] = in.planes[c].back();
        phase_ = src_pos - static_cast<double>(in.num_samples());
    }

    return out;
}

// ── Channel router ────────────────────────────────────────────────────────────

ChannelRouterMatrix ChannelRouterMatrix::identity(std::uint32_t ch) {
    ChannelRouterMatrix m;
    m.src_channels = ch; m.dst_channels = ch;
    m.gains.assign(ch, std::vector<float>(ch, 0.f));
    for (std::uint32_t i = 0; i < ch; ++i) m.gains[i][i] = 1.f;
    return m;
}

ChannelRouterMatrix ChannelRouterMatrix::stereo_to_mono() {
    ChannelRouterMatrix m;
    m.src_channels = 2; m.dst_channels = 1;
    m.gains = {{0.5f, 0.5f}};
    return m;
}

ChannelRouterMatrix ChannelRouterMatrix::mono_to_stereo() {
    ChannelRouterMatrix m;
    m.src_channels = 1; m.dst_channels = 2;
    m.gains = {{1.f}, {1.f}};
    return m;
}

AudioBuffer route_channels(const AudioBuffer& src, const ChannelRouterMatrix& mx) {
    const std::size_t n = src.num_samples();
    AudioBuffer out = AudioBuffer::silent(src.sample_rate, mx.dst_channels, n, src.format);
    out.pts = src.pts;
    for (std::uint32_t d = 0; d < mx.dst_channels; ++d)
        for (std::uint32_t s = 0; s < mx.src_channels && s < src.num_channels; ++s)
            for (std::size_t i = 0; i < n; ++i)
                out.planes[d][i] += mx.gains[d][s] * src.planes[s][i];
    return out;
}

// ── Mixer ─────────────────────────────────────────────────────────────────────

AudioBuffer mix(const std::vector<AudioBuffer>& inputs, const std::vector<float>& gains) {
    if (inputs.empty()) return {};
    const std::size_t n  = inputs[0].num_samples();
    const std::uint32_t ch = inputs[0].num_channels;
    AudioBuffer out = AudioBuffer::silent(inputs[0].sample_rate, ch, n, inputs[0].format);
    out.pts = inputs[0].pts;
    for (std::size_t bi = 0; bi < inputs.size(); ++bi) {
        const float g = (bi < gains.size()) ? gains[bi] : 1.f;
        const auto& buf = inputs[bi];
        for (std::uint32_t c = 0; c < std::min(ch, buf.num_channels); ++c)
            for (std::size_t i = 0; i < std::min(n, buf.num_samples()); ++i)
                out.planes[c][i] += g * buf.planes[c][i];
    }
    // Clip
    for (auto& ch_data : out.planes)
        for (auto& s : ch_data)
            s = std::max(-1.f, std::min(1.f, s));
    return out;
}

// ── Normalisation ─────────────────────────────────────────────────────────────

AudioBuffer peak_normalize(const AudioBuffer& src, float target_peak) {
    float peak = 0.f;
    for (const auto& ch : src.planes)
        for (float s : ch) peak = std::max(peak, std::abs(s));
    if (peak < 1e-9f) return src;
    const float gain = target_peak / peak;
    AudioBuffer out = src.clone();
    for (auto& ch : out.planes)
        for (auto& s : ch) s *= gain;
    return out;
}

AudioBuffer rms_normalize(const AudioBuffer& src, float target_rms) {
    float sq_sum = 0.f; std::size_t cnt = 0;
    for (const auto& ch : src.planes)
        for (float s : ch) { sq_sum += s * s; ++cnt; }
    const float rms = (cnt > 0) ? std::sqrt(sq_sum / cnt) : 0.f;
    if (rms < 1e-9f) return src;
    const float gain = target_rms / rms;
    AudioBuffer out = src.clone();
    for (auto& ch : out.planes)
        for (auto& s : ch) s = std::max(-1.f, std::min(1.f, s * gain));
    return out;
}

AudioBuffer apply_gain_envelope(const AudioBuffer& src, const std::vector<float>& g) {
    if (g.size() != src.num_samples())
        throw std::invalid_argument("gains size must match num_samples");
    AudioBuffer out = src.clone();
    for (auto& ch : out.planes)
        for (std::size_t i = 0; i < ch.size(); ++i) ch[i] *= g[i];
    return out;
}

// ── Fades ─────────────────────────────────────────────────────────────────────

AudioBuffer fade_in(const AudioBuffer& src, std::size_t fade_samples) {
    AudioBuffer out = src.clone();
    const std::size_t n = std::min(fade_samples, src.num_samples());
    for (auto& ch : out.planes)
        for (std::size_t i = 0; i < n; ++i)
            ch[i] *= static_cast<float>(i) / n;
    return out;
}

AudioBuffer fade_out(const AudioBuffer& src, std::size_t fade_samples) {
    AudioBuffer out = src.clone();
    const std::size_t total = src.num_samples();
    const std::size_t n = std::min(fade_samples, total);
    const std::size_t start = total - n;
    for (auto& ch : out.planes)
        for (std::size_t i = 0; i < n; ++i)
            ch[start + i] *= static_cast<float>(n - 1 - i) / n;
    return out;
}

} // namespace audio
} // namespace trekker
