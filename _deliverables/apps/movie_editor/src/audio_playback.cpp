#ifndef NOMINMAX
#define NOMINMAX
#endif
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "audio_playback.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace movie_editor_app {
namespace {
constexpr ma_uint32 rate = 48000;
constexpr ma_uint32 channels = 2;
}

struct AudioPlayback::Impl {
    ma_context context{};
    ma_device device{};
    ma_pcm_rb ring{};
    bool context_ready = false, device_ready = false, ring_ready = false;
    std::atomic<bool> stopping{false};
    std::atomic<bool> device_stopped{false};
    std::atomic<std::uint64_t> consumed{0};
    std::int64_t start_us = 0, end_us = 0;
    std::uint64_t latency_frames = 0;
    std::thread producer;
    mutable std::mutex error_mutex;
    std::string error;

    static void callback(ma_device* device, void* output, const void*, ma_uint32 frames) {
        auto& self = *static_cast<Impl*>(device->pUserData);
        auto* samples = static_cast<float*>(output);
        std::fill(samples, samples + frames * channels, 0.0f);
        ma_uint32 copied = 0;
        while (copied < frames) {
            ma_uint32 available = frames - copied;
            void* source = nullptr;
            ma_pcm_rb_acquire_read(&self.ring, &available, &source);
            if (available == 0) break;
            std::memcpy(samples + copied * channels, source, available * channels * sizeof(float));
            ma_pcm_rb_commit_read(&self.ring, available);
            copied += available;
        }
        // Underflow silence does not move the video clock ahead of the audio.
        self.consumed.fetch_add(copied);
    }

    static void notification(const ma_device_notification* event) {
        auto& self = *static_cast<Impl*>(event->pDevice->pUserData);
        if (event->type == ma_device_notification_type_stopped && !self.stopping.load())
            self.device_stopped.store(true);
    }

    void write(const trekker::audio::AudioBuffer& audio) {
        std::size_t offset = 0;
        while (offset < audio.num_samples() && !stopping.load()) {
            ma_uint32 count = static_cast<ma_uint32>(audio.num_samples() - offset);
            void* output = nullptr;
            ma_pcm_rb_acquire_write(&ring, &count, &output);
            if (!count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            auto* samples = static_cast<float*>(output);
            for (ma_uint32 i = 0; i < count; ++i) {
                for (ma_uint32 c = 0; c < channels; ++c)
                    samples[i * channels + c] = std::clamp(audio.planes[c][offset + i], -1.0f, 1.0f);
            }
            ma_pcm_rb_commit_write(&ring, count);
            offset += count;
        }
    }
};

AudioPlayback::AudioPlayback() : impl_(std::make_unique<Impl>()) {}
AudioPlayback::~AudioPlayback() { stop(); }

void AudioPlayback::stop() {
    impl_->stopping.store(true);
    if (impl_->device_ready) { ma_device_uninit(&impl_->device); impl_->device_ready = false; }
    if (impl_->producer.joinable()) impl_->producer.join();
    if (impl_->context_ready) { ma_context_uninit(&impl_->context); impl_->context_ready = false; }
    if (impl_->ring_ready) { ma_pcm_rb_uninit(&impl_->ring); impl_->ring_ready = false; }
}

void AudioPlayback::start(trekker::movie_editor::EditorProject project, std::int64_t position_us, Device selection) {
    stop();
    impl_->stopping.store(false);
    impl_->device_stopped.store(false);
    impl_->consumed.store(0);
    impl_->start_us = position_us;
    impl_->end_us = project.duration_us();
    {
        std::lock_guard<std::mutex> guard(impl_->error_mutex);
        impl_->error.clear();
    }
    if (position_us < 0 || position_us >= impl_->end_us) throw std::invalid_argument("Playback position is outside the timeline.");
    try {
        const ma_backend null_backend = ma_backend_null;
        if (ma_context_init(selection == Device::NullForTests ? &null_backend : nullptr,
                            selection == Device::NullForTests ? 1 : 0, nullptr, &impl_->context) != MA_SUCCESS)
            throw std::runtime_error("Audio backend initialization failed.");
        impl_->context_ready = true;
        if (selection == Device::System && impl_->context.backend == ma_backend_null)
            throw std::runtime_error("No system audio device is available. On Linux install the PulseAudio or ALSA runtime and check your audio server.");
        if (ma_pcm_rb_init(ma_format_f32, channels, rate, nullptr, nullptr, &impl_->ring) != MA_SUCCESS)
            throw std::runtime_error("Cannot allocate playback audio buffer.");
        impl_->ring_ready = true;
        auto config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = channels;
        config.sampleRate = rate;
        config.periodSizeInMilliseconds = 10;
        config.periods = 2;
        config.dataCallback = Impl::callback;
        config.notificationCallback = Impl::notification;
        config.pUserData = impl_.get();
        if (ma_device_init(&impl_->context, &config, &impl_->device) != MA_SUCCESS)
            throw std::runtime_error("Cannot open the system audio output device.");
        impl_->device_ready = true;
        impl_->latency_frames = static_cast<std::uint64_t>(impl_->device.playback.internalPeriodSizeInFrames) *
            impl_->device.playback.internalPeriods * rate /
            std::max<ma_uint32>(1, impl_->device.playback.internalSampleRate);
        const auto first_end = std::min(impl_->end_us, position_us + 100'000);
        impl_->write(project.render_audio_mix(position_us, first_end, rate, channels));
        impl_->producer = std::thread([this, project = std::move(project), first_end]() mutable {
            try {
                for (auto at = first_end; at < impl_->end_us && !impl_->stopping.load();) {
                    const auto end = std::min(impl_->end_us, at + 100'000);
                    impl_->write(project.render_audio_mix(at, end, rate, channels));
                    at = end;
                }
                // Drain the backend's queued audio instead of cutting off the final buffers.
                impl_->write(trekker::audio::AudioBuffer::silent(rate, channels,
                    static_cast<std::size_t>(impl_->latency_frames)));
            } catch (const std::exception& error) {
                std::lock_guard<std::mutex> guard(impl_->error_mutex);
                impl_->error = error.what();
            }
        });
        if (ma_device_start(&impl_->device) != MA_SUCCESS) throw std::runtime_error("Cannot start audio playback.");
    } catch (...) {
        stop();
        throw;
    }
}

std::int64_t AudioPlayback::position_us() const {
    const auto submitted = impl_->consumed.load();
    const auto audible = submitted > impl_->latency_frames ? submitted - impl_->latency_frames : 0;
    return std::min(impl_->end_us, impl_->start_us +
        static_cast<std::int64_t>(audible * 1'000'000 / rate));
}
bool AudioPlayback::finished() const { return position_us() >= impl_->end_us - 21; }
void AudioPlayback::check_error() const {
    if (impl_->device_stopped.load()) throw std::runtime_error("Audio output stopped unexpectedly. Check your output device and press Play to retry.");
    std::lock_guard<std::mutex> guard(impl_->error_mutex);
    if (!impl_->error.empty()) throw std::runtime_error("Audio mixing failed: " + impl_->error);
}
}
