#include "cli_tools.hpp"
#include "full_application_window.hpp"
#include "ios_chrome.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <ctime>
#include <vector>

#if defined(APPLE)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace {

struct Options {
    bool list_mics = false;
    bool cli_mode = false;
    std::string output_path = "recording_studio_output.wav";
    std::string microphone_name;
};

Options parse_args(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("recording_studio");
    parser.set_description("Record WAV audio from attached microphones using app_builder window shell.");

    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"list-mics", 'l', false, false, "", "List detected input microphones and exit."});
    parser.add_option({"cli", '\0', false, false, "", "Use CLI mode instead of launching a window."});
    parser.add_option({"output", 'o', true, false, "PATH", "Output WAV path."});
    parser.add_option({"mic", 'm', true, false, "NAME", "Preferred microphone name (exact match)."});

    const auto result = parser.parse_argv(argc, argv);
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            std::cerr << "Error: " << error << "\n";
        }
        std::cerr << "\n" << parser.render_help() << "\n";
        std::exit(1);
    }

    if (result.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        std::exit(0);
    }

    Options options;
    options.list_mics = result.has_option("list-mics");
    options.cli_mode = result.has_option("cli");
    options.output_path = result.option_value("output", "recording_studio_output.wav");
    options.microphone_name = result.option_value("mic", "");
    return options;
}

void write_wav_file(const std::string& output_path,
                    const std::vector<int16_t>& pcm,
                    uint32_t sample_rate,
                    uint16_t channel_count) {
    const uint16_t bits_per_sample = 16;
    const uint32_t byte_rate = sample_rate * channel_count * (bits_per_sample / 8);
    const uint16_t block_align = static_cast<uint16_t>(channel_count * (bits_per_sample / 8));
    const uint32_t data_size = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
    const uint32_t riff_size = 36u + data_size;

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open output WAV path: " + output_path);
    }

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&riff_size), sizeof(riff_size));
    out.write("WAVE", 4);

    const uint32_t fmt_chunk_size = 16;
    const uint16_t audio_format_pcm = 1;
    out.write("fmt ", 4);
    out.write(reinterpret_cast<const char*>(&fmt_chunk_size), sizeof(fmt_chunk_size));
    out.write(reinterpret_cast<const char*>(&audio_format_pcm), sizeof(audio_format_pcm));
    out.write(reinterpret_cast<const char*>(&channel_count), sizeof(channel_count));
    out.write(reinterpret_cast<const char*>(&sample_rate), sizeof(sample_rate));
    out.write(reinterpret_cast<const char*>(&byte_rate), sizeof(byte_rate));
    out.write(reinterpret_cast<const char*>(&block_align), sizeof(block_align));
    out.write(reinterpret_cast<const char*>(&bits_per_sample), sizeof(bits_per_sample));

    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    out.write(reinterpret_cast<const char*>(pcm.data()), static_cast<std::streamsize>(data_size));
}

std::string make_timestamped_output_path() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &raw_time);
#else
    local_tm = *std::localtime(&raw_time);
#endif
    std::ostringstream out;
    out << "recording_" << std::put_time(&local_tm, "%Y%m%d_%H%M%S") << ".wav";
    return out.str();
}

std::string format_seconds(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }
    const auto total_seconds = static_cast<long long>(seconds);
    const long long mins = total_seconds / 60;
    const long long secs = total_seconds % 60;
    std::ostringstream out;
    out << mins << ':' << std::setw(2) << std::setfill('0') << secs;
    return out.str();
}

#if defined(APPLE)

std::string cfstring_to_utf8(CFStringRef value) {
    if (value == nullptr) {
        return "";
    }

    const CFIndex length = CFStringGetLength(value);
    const CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::string out(static_cast<std::size_t>(max_size), '\0');
    if (!CFStringGetCString(value, out.data(), max_size, kCFStringEncodingUTF8)) {
        return "";
    }
    out.resize(std::strlen(out.c_str()));
    return out;
}

bool device_has_input_scope(AudioDeviceID device_id) {
    AudioObjectPropertyAddress streams_address{
        kAudioDevicePropertyStreams,
        kAudioObjectPropertyScopeInput,
        kAudioObjectPropertyElementMain
    };

    UInt32 data_size = 0;
    const OSStatus size_status = AudioObjectGetPropertyDataSize(device_id, &streams_address, 0, nullptr, &data_size);
    if (size_status != noErr || data_size == 0) {
        return false;
    }

    const auto stream_count = data_size / static_cast<UInt32>(sizeof(AudioStreamID));
    return stream_count > 0;
}

std::string device_name(AudioDeviceID device_id) {
    AudioObjectPropertyAddress name_address{
        kAudioObjectPropertyName,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    CFStringRef name_ref = nullptr;
    UInt32 data_size = sizeof(name_ref);
    const OSStatus status = AudioObjectGetPropertyData(device_id, &name_address, 0, nullptr, &data_size, &name_ref);
    if (status != noErr || name_ref == nullptr) {
        return "<unknown microphone>";
    }

    const std::string name = cfstring_to_utf8(name_ref);
    CFRelease(name_ref);
    return name.empty() ? "<unknown microphone>" : name;
}

std::string device_uid(AudioDeviceID device_id) {
    AudioObjectPropertyAddress uid_address{
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    CFStringRef uid_ref = nullptr;
    UInt32 data_size = sizeof(uid_ref);
    const OSStatus status = AudioObjectGetPropertyData(device_id, &uid_address, 0, nullptr, &data_size, &uid_ref);
    if (status != noErr || uid_ref == nullptr) {
        return "";
    }

    const std::string uid = cfstring_to_utf8(uid_ref);
    CFRelease(uid_ref);
    return uid;
}

std::vector<AudioDeviceID> list_microphone_device_ids() {
    AudioObjectPropertyAddress devices_address{
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    UInt32 data_size = 0;
    const OSStatus size_status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                                                 &devices_address,
                                                                 0,
                                                                 nullptr,
                                                                 &data_size);
    if (size_status != noErr || data_size == 0) {
        return {};
    }

    std::vector<AudioDeviceID> all_devices(data_size / sizeof(AudioDeviceID));
    OSStatus devices_status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                         &devices_address,
                                                         0,
                                                         nullptr,
                                                         &data_size,
                                                         all_devices.data());
    if (devices_status != noErr) {
        return {};
    }

    std::vector<AudioDeviceID> microphones;
    microphones.reserve(all_devices.size());
    for (const auto device_id : all_devices) {
        if (device_has_input_scope(device_id)) {
            microphones.push_back(device_id);
        }
    }
    return microphones;
}

AudioDeviceID default_input_device() {
    AudioObjectPropertyAddress default_input_address{
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    AudioDeviceID default_device = kAudioObjectUnknown;
    UInt32 data_size = sizeof(default_device);
    const OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                       &default_input_address,
                                                       0,
                                                       nullptr,
                                                       &data_size,
                                                       &default_device);
    if (status != noErr) {
        return kAudioObjectUnknown;
    }
    return default_device;
}

AudioDeviceID choose_microphone_device(const std::string& preferred_name) {
    const auto devices = list_microphone_device_ids();
    if (devices.empty()) {
        return kAudioObjectUnknown;
    }

    if (!preferred_name.empty()) {
        for (const auto device_id : devices) {
            if (device_name(device_id) == preferred_name) {
                return device_id;
            }
        }
    }

    const AudioDeviceID default_device = default_input_device();
    if (default_device != kAudioObjectUnknown &&
        std::find(devices.begin(), devices.end(), default_device) != devices.end()) {
        return default_device;
    }

    return devices.front();
}

double microphone_nominal_sample_rate(AudioDeviceID device_id) {
    AudioObjectPropertyAddress nominal_rate_address{
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    Float64 sample_rate = 44100.0;
    UInt32 data_size = sizeof(sample_rate);
    const OSStatus status = AudioObjectGetPropertyData(device_id,
                                                       &nominal_rate_address,
                                                       0,
                                                       nullptr,
                                                       &data_size,
                                                       &sample_rate);
    if (status != noErr || sample_rate < 8000.0 || sample_rate > 192000.0) {
        return 44100.0;
    }
    return sample_rate;
}

class MicrophoneRecorder {
public:
    struct Device {
        std::string name;
        std::string uid;
    };

    std::vector<Device> list_microphones() const {
        std::vector<Device> devices;
        for (const auto device_id : list_microphone_device_ids()) {
            devices.push_back(Device{device_name(device_id), device_uid(device_id)});
        }
        return devices;
    }

    bool start(const std::string& output_path, const std::string& preferred_microphone, std::string& out_error) {
        if (recording_.load()) {
            out_error = "A recording session is already running.";
            return false;
        }

        const AudioDeviceID microphone = choose_microphone_device(preferred_microphone);
        if (microphone == kAudioObjectUnknown) {
            out_error = "No input microphone was detected.";
            return false;
        }

        sample_rate_ = static_cast<uint32_t>(microphone_nominal_sample_rate(microphone));
        output_path_ = output_path;
        selected_microphone_name_ = device_name(microphone);
        selected_microphone_uid_ = device_uid(microphone);

        {
            std::lock_guard<std::mutex> lock(samples_mutex_);
            samples_.clear();
        }

        AudioStreamBasicDescription format{};
        format.mSampleRate = static_cast<Float64>(sample_rate_);
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        format.mBitsPerChannel = 16;
        format.mChannelsPerFrame = channels_;
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = (format.mBitsPerChannel / 8) * format.mChannelsPerFrame;
        format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;

        const OSStatus queue_status = AudioQueueNewInput(&format,
                                                         &MicrophoneRecorder::audio_input_callback,
                                                         this,
                                                         nullptr,
                                                         kCFRunLoopCommonModes,
                                                         0,
                                                         &queue_);
        if (queue_status != noErr || queue_ == nullptr) {
            out_error = "Unable to initialize audio input queue.";
            cleanup_queue();
            return false;
        }

        if (!selected_microphone_uid_.empty()) {
            CFStringRef uid_ref = CFStringCreateWithCString(kCFAllocatorDefault,
                                                            selected_microphone_uid_.c_str(),
                                                            kCFStringEncodingUTF8);
            if (uid_ref != nullptr) {
                const OSStatus set_dev_status = AudioQueueSetProperty(queue_,
                                                                      kAudioQueueProperty_CurrentDevice,
                                                                      &uid_ref,
                                                                      static_cast<UInt32>(sizeof(uid_ref)));
                CFRelease(uid_ref);
                if (set_dev_status != noErr) {
                    out_error = "Unable to bind recording queue to selected microphone.";
                    cleanup_queue();
                    return false;
                }
            }
        }

        constexpr UInt32 k_buffer_byte_size = 4096;
        for (int i = 0; i < 3; ++i) {
            AudioQueueBufferRef buffer = nullptr;
            const OSStatus alloc_status = AudioQueueAllocateBuffer(queue_, k_buffer_byte_size, &buffer);
            if (alloc_status != noErr || buffer == nullptr) {
                out_error = "Unable to allocate audio queue buffer.";
                cleanup_queue();
                return false;
            }
            buffer->mAudioDataByteSize = k_buffer_byte_size;
            const OSStatus enqueue_status = AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr);
            if (enqueue_status != noErr) {
                out_error = "Unable to enqueue audio buffer.";
                cleanup_queue();
                return false;
            }
        }

        const OSStatus start_status = AudioQueueStart(queue_, nullptr);
        if (start_status != noErr) {
            out_error = "Unable to start audio queue capture.";
            cleanup_queue();
            return false;
        }

        recording_.store(true);
        return true;
    }

    bool stop(std::string& out_error) {
        if (!recording_.load()) {
            out_error = "Recording is not active.";
            return false;
        }

        recording_.store(false);

        if (queue_ != nullptr) {
            const OSStatus stop_status = AudioQueueStop(queue_, true);
            if (stop_status != noErr) {
                out_error = "Failed while stopping audio capture.";
                cleanup_queue();
                return false;
            }
        }

        cleanup_queue();

        return true;
    }

    bool save(const std::string& output_path, std::string& out_error) const {
        std::vector<int16_t> captured;
        {
            std::lock_guard<std::mutex> lock(samples_mutex_);
            captured = samples_;
        }

        if (captured.empty()) {
            out_error = "No audio samples available to save.";
            return false;
        }

        try {
            write_wav_file(output_path, captured, sample_rate_, channels_);
        } catch (const std::exception& ex) {
            out_error = ex.what();
            return false;
        }

        return true;
    }

    bool is_recording() const {
        return recording_.load();
    }

    std::size_t captured_samples() const {
        std::lock_guard<std::mutex> lock(samples_mutex_);
        return samples_.size();
    }

    bool cut_range(std::size_t start_sample, std::size_t end_sample) {
        if (end_sample <= start_sample) {
            return false;
        }
        std::lock_guard<std::mutex> lock(samples_mutex_);
        if (start_sample >= samples_.size()) {
            return false;
        }
        const std::size_t bounded_end = std::min(end_sample, samples_.size());
        samples_.erase(samples_.begin() + static_cast<std::ptrdiff_t>(start_sample),
                       samples_.begin() + static_cast<std::ptrdiff_t>(bounded_end));
        return true;
    }

    uint32_t sample_rate() const {
        return sample_rate_;
    }

    const std::string& selected_microphone_name() const {
        return selected_microphone_name_;
    }

private:
    static void audio_input_callback(void* user_data,
                                     AudioQueueRef queue,
                                     AudioQueueBufferRef buffer,
                                     const AudioTimeStamp*,
                                     UInt32,
                                     const AudioStreamPacketDescription*) {
        auto* self = static_cast<MicrophoneRecorder*>(user_data);
        if (self == nullptr || buffer == nullptr) {
            return;
        }

        if (self->recording_.load() && buffer->mAudioDataByteSize > 0) {
            const auto sample_count = buffer->mAudioDataByteSize / static_cast<UInt32>(sizeof(int16_t));
            const auto* sample_data = static_cast<const int16_t*>(buffer->mAudioData);
            std::lock_guard<std::mutex> lock(self->samples_mutex_);
            self->samples_.insert(self->samples_.end(), sample_data, sample_data + sample_count);
        }

        if (self->recording_.load()) {
            buffer->mAudioDataByteSize = 4096;
            AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
        }
    }

    void cleanup_queue() {
        if (queue_ != nullptr) {
            AudioQueueDispose(queue_, true);
            queue_ = nullptr;
        }
    }

private:
    AudioQueueRef queue_ = nullptr;
    std::atomic<bool> recording_{false};
    mutable std::mutex samples_mutex_;
    std::vector<int16_t> samples_;
    uint32_t sample_rate_ = 44100;
    uint16_t channels_ = 1;
    std::string selected_microphone_name_;
    std::string selected_microphone_uid_;
};

#else

class MicrophoneRecorder {
public:
    struct Device {
        std::string name;
        std::string uid;
    };

    std::vector<Device> list_microphones() const {
        return {};
    }

    bool start(const std::string&, const std::string&, std::string& out_error) {
        out_error = "Microphone capture is currently implemented for macOS in this target.";
        return false;
    }

    bool stop(std::string& out_error) {
        out_error = "No active recording.";
        return false;
    }

    bool save(const std::string&, std::string& out_error) const {
        out_error = "Microphone capture is currently implemented for macOS in this target.";
        return false;
    }

    bool is_recording() const {
        return false;
    }

    std::size_t captured_samples() const {
        return 0;
    }

    bool cut_range(std::size_t, std::size_t) {
        return false;
    }

    uint32_t sample_rate() const {
        return 44100;
    }

    const std::string& selected_microphone_name() const {
        static const std::string none = "N/A";
        return none;
    }
};

#endif

std::string sample_count_line(const MicrophoneRecorder& recorder) {
    return "Captured samples: " + std::to_string(recorder.captured_samples());
}

void print_microphones(const std::vector<MicrophoneRecorder::Device>& devices) {
    if (devices.empty()) {
        std::cout << "No input microphones detected.\n";
        return;
    }

    std::cout << "Detected microphones:\n";
    for (std::size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i].name;
        if (!devices[i].uid.empty()) {
            std::cout << " (uid=" << devices[i].uid << ")";
        }
        std::cout << "\n";
    }
}

int run_cli(const Options& options) {
    MicrophoneRecorder recorder;
    const auto devices = recorder.list_microphones();

    if (options.list_mics) {
        print_microphones(devices);
        return 0;
    }

    std::cout << "recording_studio (CLI)\n";
    print_microphones(devices);
    std::cout << "\nPress ENTER to start recording using the selected microphone.\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string error;
    if (!recorder.start(options.output_path, options.microphone_name, error)) {
        std::cerr << "Failed to start recording: " << error << "\n";
        return 1;
    }

    std::cout << "Recording... press ENTER to stop and save to: " << options.output_path << "\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (!recorder.stop(error)) {
        std::cerr << "Failed to stop recording: " << error << "\n";
        return 1;
    }

    if (!recorder.save(options.output_path, error)) {
        std::cerr << "Failed to save recording: " << error << "\n";
        return 1;
    }

    std::cout << "Saved WAV recording: " << options.output_path << "\n";
    return 0;
}

int run_window(const Options& options) {
    using namespace graphics::full_application_window;
    using app_assets::ios_chrome::ButtonSpec;
    using app_assets::ios_chrome::ButtonTone;
    using app_assets::ios_chrome::Rect;

    MicrophoneRecorder recorder;
    std::vector<MicrophoneRecorder::Device> microphones = recorder.list_microphones();
    if (options.list_mics) {
        print_microphones(microphones);
        return 0;
    }

    WindowConfig config("recording_studio", 960, 620, true, true);
    FullApplicationWindow window(config);

    struct UiState {
        std::string status = "Ready.";
        std::string output_path;
        std::string selected_microphone;
        int selected_microphone_index = -1;
        bool recording = false;
        bool start_hover = false;
        bool stop_hover = false;
        std::size_t playhead_sample = 0;
        std::size_t mark_a_sample = 0;
        std::size_t mark_b_sample = 0;
        bool has_mark_a = false;
        bool has_mark_b = false;
        int timeline_zoom = 1;
        bool timeline_follow_playhead = true;
        int timeline_mode_hovered_index = -1;
        std::size_t timeline_view_center_sample = 0;
        bool overview_drag_active = false;
        bool mark_a_chip_hover = false;
        bool mark_b_chip_hover = false;
    };

    UiState state;
    state.output_path = options.output_path;
    if (!options.microphone_name.empty()) {
        state.selected_microphone = options.microphone_name;
        for (std::size_t i = 0; i < microphones.size(); ++i) {
            if (microphones[i].name == options.microphone_name) {
                state.selected_microphone_index = static_cast<int>(i);
                break;
            }
        }
    } else if (!microphones.empty()) {
        state.selected_microphone_index = 0;
        state.selected_microphone = microphones[0].name;
    } else {
        state.selected_microphone = "(default microphone)";
    }

    constexpr int mic_list_top = 304;
    constexpr int mic_row_height = 24;
    constexpr int mic_row_gap = 6;
    constexpr Rect mic_row_bounds_rect{32, 0, 900, 0};

    constexpr Rect timeline_rect{24, 500, 936, 534};

    constexpr Rect overview_rect{24, 428, 936, 452};

    constexpr Rect mode_segment_rect{420, 460, 552, 494};
    constexpr Rect pan_left_rect{560, 460, 632, 494};
    constexpr Rect pan_right_rect{640, 460, 712, 494};
    constexpr Rect zoom_out_rect{760, 460, 834, 494};
    constexpr Rect zoom_in_rect{842, 460, 936, 494};

    constexpr Rect start_rect{24, 190, 254, 240};
    constexpr Rect stop_rect{276, 190, 506, 240};
    constexpr Rect save_rect{528, 190, 758, 240};
    constexpr Rect refresh_rect{780, 190, 936, 240};
    constexpr Rect output_toggle_rect{24, 248, 312, 286};

    constexpr Rect mark_a_rect{24, 546, 176, 584};
    constexpr Rect mark_b_rect{188, 546, 340, 584};
    constexpr Rect cut_rect{352, 546, 504, 584};

    constexpr Rect top_controls_group_rect{20, 184, 940, 292};
    constexpr Rect timeline_controls_group_rect{412, 454, 940, 500};
    constexpr Rect editing_controls_group_rect{20, 540, 510, 590};
    constexpr Rect mark_a_chip_rect{700, 438, 810, 466};
    constexpr Rect mark_b_chip_rect{818, 438, 930, 466};

    auto mic_row_bounds = [](std::size_t index) {
        const int top = mic_list_top + static_cast<int>(index) * (mic_row_height + mic_row_gap);
        const int bottom = top + mic_row_height;
        return std::pair<int, int>{top, bottom};
    };

    auto timeline_view_bounds = [&](std::size_t total_samples) {
        if (total_samples == 0) {
            return std::pair<std::size_t, std::size_t>{0, 0};
        }

        const std::size_t zoom = static_cast<std::size_t>(std::max(1, state.timeline_zoom));
        const std::size_t visible_samples = std::max<std::size_t>(1, total_samples / zoom);
        const std::size_t clamped_playhead = std::min(state.playhead_sample, total_samples - 1);
        const std::size_t center_sample = state.timeline_follow_playhead
                                              ? clamped_playhead
                                              : std::min(state.timeline_view_center_sample, total_samples - 1);
        const std::size_t half = visible_samples / 2;

        std::size_t view_start = (center_sample > half) ? (center_sample - half) : 0;
        if (view_start + visible_samples > total_samples) {
            view_start = total_samples - visible_samples;
        }

        const std::size_t view_end = std::min(total_samples, view_start + visible_samples);
        return std::pair<std::size_t, std::size_t>{view_start, view_end};
    };

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent&) {
        int width = 0;
        int height = 0;
        if (!window.client_size(width, height)) {
            width = 960;
            height = 620;
        }

        window.clear_background(20, 26, 36);
        window.draw_text_line(24, 24, "Recording Studio");
        window.draw_text_line(24, 52, "Output WAV: " + state.output_path);
        window.draw_text_line(24, 76, "Selected microphone: " + state.selected_microphone);
        window.draw_text_line(24, 100, "Active device: " + recorder.selected_microphone_name());
        window.draw_text_line(24, 124, sample_count_line(recorder));
        window.draw_text_line(24, 148, "Status: " + state.status);

        app_assets::ios_chrome::draw_button_group(window, top_controls_group_rect);
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{start_rect,
                                                           "Start Recording",
                                                           ButtonTone::Success,
                                                           !state.recording,
                                                           state.start_hover,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{stop_rect,
                                                           "Stop + Save",
                                                           ButtonTone::Danger,
                                                           state.recording,
                                                           state.stop_hover,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{save_rect,
                                                           "Save WAV",
                                                           ButtonTone::Primary,
                                                           true,
                                                           false,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{refresh_rect,
                                                           "Refresh",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{output_toggle_rect,
                                                           "Toggle Output Filename",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           state.output_path.rfind("recording_", 0) == 0});

        int y = 280;
        window.draw_text_line(24, y, "Detected Microphones:");
        y += 24;
        if (microphones.empty()) {
            window.draw_text_line(40, y, "(none detected)");
        } else {
            for (std::size_t i = 0; i < microphones.size(); ++i) {
                const auto [row_top, row_bottom] = mic_row_bounds(i);
                if (row_bottom > height - 40) {
                    break;
                }

                const bool selected = static_cast<int>(i) == state.selected_microphone_index;
                const Rect mic_row_rect{mic_row_bounds_rect.left,
                                        row_top,
                                        mic_row_bounds_rect.right,
                                        row_bottom};
                app_assets::ios_chrome::draw_ios_button(window,
                                                        ButtonSpec{mic_row_rect,
                                                                   std::to_string(i + 1) + ". " + microphones[i].name,
                                                                   selected ? ButtonTone::Primary : ButtonTone::Neutral,
                                                                   true,
                                                                   false,
                                                                   selected});
            }

            window.draw_text_line(24, height - 54, "Click a microphone row to select it.");
        }

        const std::size_t total_samples = recorder.captured_samples();
        const double sr = static_cast<double>(recorder.sample_rate());
        const double total_seconds = (sr > 0.0) ? static_cast<double>(total_samples) / sr : 0.0;
        const std::size_t clamped_playhead = std::min(state.playhead_sample, total_samples);
        const double playhead_seconds = (sr > 0.0) ? static_cast<double>(clamped_playhead) / sr : 0.0;

        window.draw_text_line(24, 410, "Overview (drag viewport to pan)");
        window.fill_rect(overview_rect.left, overview_rect.top, overview_rect.right, overview_rect.bottom, 45, 54, 68);

        const int overview_width = overview_rect.right - overview_rect.left;
        const int timeline_width = timeline_rect.right - timeline_rect.left;
        const auto [view_start, view_end] = timeline_view_bounds(total_samples);
        const std::size_t view_span_samples = std::max<std::size_t>(1, view_end - view_start);

        if (total_samples > 0) {
            const int overview_play_x = overview_rect.left + static_cast<int>((static_cast<double>(clamped_playhead) /
                                                                                static_cast<double>(total_samples)) * overview_width);
            window.fill_rect(overview_play_x - 1,
                             overview_rect.top,
                             overview_play_x + 1,
                             overview_rect.bottom,
                             242,
                             224,
                             92);

            const int view_left_x = overview_rect.left + static_cast<int>((static_cast<double>(view_start) /
                                                                           static_cast<double>(total_samples)) * overview_width);
            const int view_right_x = overview_rect.left + static_cast<int>((static_cast<double>(view_end) /
                                                                            static_cast<double>(total_samples)) * overview_width);
            window.fill_rect(view_left_x, overview_rect.top, view_right_x, overview_rect.bottom, 78, 126, 176);
        }

        window.draw_text_line(24, 468, "Timeline (click to jump playhead)");
        app_assets::ios_chrome::draw_button_group(window, timeline_controls_group_rect);
        const app_assets::ios_chrome::SegmentedControlSpec mode_segment{
            mode_segment_rect,
            {"Follow", "Manual"},
            state.timeline_follow_playhead ? 0 : 1,
            state.timeline_mode_hovered_index,
            true,
        };
        app_assets::ios_chrome::draw_segmented_control(window, mode_segment);
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{pan_left_rect,
                                                           "Pan<",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{pan_right_rect,
                                                           "Pan>",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{zoom_out_rect,
                                                           "Zoom-",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           false});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{zoom_in_rect,
                                                           "Zoom+",
                                                           ButtonTone::Neutral,
                                                           true,
                                                           false,
                                                           false});

        window.fill_rect(timeline_rect.left, timeline_rect.top, timeline_rect.right, timeline_rect.bottom, 50, 60, 78);

        const std::size_t local_playhead = (clamped_playhead <= view_start)
                                               ? 0
                                               : std::min(view_span_samples, clamped_playhead - view_start);
          const int play_x = timeline_rect.left + static_cast<int>((total_samples > 0)
                                                                                            ? (static_cast<double>(local_playhead) /
                                                                                                static_cast<double>(view_span_samples)) * timeline_width
                                                                                            : 0.0);
          window.fill_rect(play_x - 1, timeline_rect.top, play_x + 1, timeline_rect.bottom, 242, 224, 92);

        if (state.has_mark_a && total_samples > 0 && state.mark_a_sample >= view_start && state.mark_a_sample <= view_end) {
            const std::size_t local_a = state.mark_a_sample - view_start;
            const int a_x = timeline_rect.left + static_cast<int>((static_cast<double>(local_a) /
                                                                   static_cast<double>(view_span_samples)) * timeline_width);
            window.fill_rect(a_x - 1, timeline_rect.top - 8, a_x + 1, timeline_rect.bottom + 8, 104, 206, 114);
        }
        if (state.has_mark_b && total_samples > 0 && state.mark_b_sample >= view_start && state.mark_b_sample <= view_end) {
            const std::size_t local_b = state.mark_b_sample - view_start;
            const int b_x = timeline_rect.left + static_cast<int>((static_cast<double>(local_b) /
                                                                   static_cast<double>(view_span_samples)) * timeline_width);
            window.fill_rect(b_x - 1, timeline_rect.top - 8, b_x + 1, timeline_rect.bottom + 8, 216, 108, 108);
        }

        app_assets::ios_chrome::draw_button_group(window, editing_controls_group_rect);
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{mark_a_rect,
                                                           "Mark A",
                                                           ButtonTone::Success,
                                                           true,
                                                           false,
                                                           state.has_mark_a});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{mark_b_rect,
                                                           "Mark B",
                                                           ButtonTone::Danger,
                                                           true,
                                                           false,
                                                           state.has_mark_b});
        app_assets::ios_chrome::draw_ios_button(window,
                                                ButtonSpec{cut_rect,
                                                           "Cut A-B",
                                                           ButtonTone::Danger,
                                                           state.has_mark_a && state.has_mark_b && state.mark_a_sample != state.mark_b_sample,
                                                           false,
                                                           false});

        const double view_start_seconds = (sr > 0.0) ? static_cast<double>(view_start) / sr : 0.0;
        const double view_end_seconds = (sr > 0.0) ? static_cast<double>(view_end) / sr : 0.0;
        window.draw_text_line(24, 444,
                      "Zoom " + std::to_string(state.timeline_zoom) + "x, view " +
                      format_seconds(view_start_seconds) + " - " + format_seconds(view_end_seconds));
        window.draw_text_line(520, 444,
                      std::string("Mode: ") + (state.timeline_follow_playhead ? "Follow" : "Manual Pan"));

        app_assets::ios_chrome::draw_toggle_chip(
            window,
            app_assets::ios_chrome::ToggleChipSpec{mark_a_chip_rect,
                                                   "Mark A",
                                                   state.has_mark_a,
                                                   state.mark_a_chip_hover,
                                                   true});
        app_assets::ios_chrome::draw_toggle_chip(
            window,
            app_assets::ios_chrome::ToggleChipSpec{mark_b_chip_rect,
                                                   "Mark B",
                                                   state.has_mark_b,
                                                   state.mark_b_chip_hover,
                                                   true});

        window.draw_text_line(24, 590, "Playhead: " + format_seconds(playhead_seconds) +
                                       " / " + format_seconds(total_seconds));
        window.draw_text_line(24, height - 30, "Close the window to exit.");
    };

    window.set_render_hooks(std::move(hooks));
    if (!window.create()) {
        std::cerr << "Failed to create recording_studio window.\n";
        return 1;
    }

    window.show();

    bool previous_left_down = false;
    while (window.is_open()) {
        if (!window.pump_events()) {
            break;
        }

        PointerState pointer{};
        if (window.query_pointer_state(pointer)) {
            const bool start_hit = pointer.inside && app_assets::ios_chrome::contains(start_rect, pointer.x, pointer.y);
            const bool stop_hit = pointer.inside && app_assets::ios_chrome::contains(stop_rect, pointer.x, pointer.y);
            const bool save_hit = pointer.inside && app_assets::ios_chrome::contains(save_rect, pointer.x, pointer.y);
            const bool refresh_hit = pointer.inside && app_assets::ios_chrome::contains(refresh_rect, pointer.x, pointer.y);
            const bool output_toggle_hit = pointer.inside && app_assets::ios_chrome::contains(output_toggle_rect, pointer.x, pointer.y);

            const bool timeline_hit = pointer.inside && app_assets::ios_chrome::contains(timeline_rect, pointer.x, pointer.y);
            const bool overview_hit = pointer.inside && app_assets::ios_chrome::contains(overview_rect, pointer.x, pointer.y);
            const bool zoom_out_hit = pointer.inside && app_assets::ios_chrome::contains(zoom_out_rect, pointer.x, pointer.y);
            const bool zoom_in_hit = pointer.inside && app_assets::ios_chrome::contains(zoom_in_rect, pointer.x, pointer.y);
            const bool pan_left_hit = pointer.inside && app_assets::ios_chrome::contains(pan_left_rect, pointer.x, pointer.y);
            const bool pan_right_hit = pointer.inside && app_assets::ios_chrome::contains(pan_right_rect, pointer.x, pointer.y);
            const app_assets::ios_chrome::SegmentedControlSpec mode_segment{
                mode_segment_rect,
                {"Follow", "Manual"},
                state.timeline_follow_playhead ? 0 : 1,
                -1,
                true,
            };
            const int mode_segment_hit_index = pointer.inside
                                                   ? app_assets::ios_chrome::segmented_index_at(mode_segment, pointer.x, pointer.y)
                                                   : -1;

            state.timeline_mode_hovered_index = mode_segment_hit_index;
            state.mark_a_chip_hover = pointer.inside && app_assets::ios_chrome::contains(mark_a_chip_rect, pointer.x, pointer.y);
            state.mark_b_chip_hover = pointer.inside && app_assets::ios_chrome::contains(mark_b_chip_rect, pointer.x, pointer.y);

            const bool mark_a_hit = pointer.inside && app_assets::ios_chrome::contains(mark_a_rect, pointer.x, pointer.y);
            const bool mark_b_hit = pointer.inside && app_assets::ios_chrome::contains(mark_b_rect, pointer.x, pointer.y);
            const bool cut_hit = pointer.inside && app_assets::ios_chrome::contains(cut_rect, pointer.x, pointer.y);
            const bool just_clicked = pointer.left_button_down && !previous_left_down;

            auto update_center_from_overview_x = [&](int x) {
                const std::size_t total_samples = recorder.captured_samples();
                if (total_samples == 0) {
                    return;
                }
                const int span = std::max(1, overview_rect.right - overview_rect.left);
                const int rel_x = std::clamp(x - overview_rect.left, 0, span);
                const double t = static_cast<double>(rel_x) / static_cast<double>(span);
                state.timeline_follow_playhead = false;
                state.timeline_view_center_sample = static_cast<std::size_t>(t * static_cast<double>(total_samples));
            };

            state.start_hover = start_hit;
            state.stop_hover = stop_hit;

            if (just_clicked && start_hit && !state.recording) {
                std::string error;
                const std::string selected_name =
                    (state.selected_microphone_index >= 0 &&
                     static_cast<std::size_t>(state.selected_microphone_index) < microphones.size())
                        ? microphones[static_cast<std::size_t>(state.selected_microphone_index)].name
                        : state.selected_microphone;

                if (recorder.start(options.output_path, selected_name, error)) {
                    state.recording = true;
                    state.status = "Recording in progress...";
                } else {
                    state.status = "Start failed: " + error;
                }
                window.request_redraw();
            }

            if (just_clicked && stop_hit && state.recording) {
                std::string error;
                if (recorder.stop(error)) {
                    state.recording = false;
                    state.status = "Recording stopped. You can cut timeline and save WAV.";
                } else {
                    state.status = "Stop failed: " + error;
                }
                window.request_redraw();
            }

            if (just_clicked && save_hit) {
                std::string error;
                if (recorder.save(state.output_path, error)) {
                    state.status = "Saved recording to " + state.output_path;
                } else {
                    state.status = "Save failed: " + error;
                }
                window.request_redraw();
            }

            if (just_clicked && refresh_hit) {
                microphones = recorder.list_microphones();
                if (microphones.empty()) {
                    state.selected_microphone_index = -1;
                    state.selected_microphone = "(default microphone)";
                    state.status = "No input microphones detected.";
                } else {
                    if (state.selected_microphone_index < 0 ||
                        static_cast<std::size_t>(state.selected_microphone_index) >= microphones.size()) {
                        state.selected_microphone_index = 0;
                        state.selected_microphone = microphones[0].name;
                    }
                    state.status = "Microphone list refreshed.";
                }
                window.request_redraw();
            }

            if (just_clicked && output_toggle_hit) {
                if (state.output_path.rfind("recording_", 0) == 0) {
                    state.output_path = "recording_studio_output.wav";
                } else {
                    state.output_path = make_timestamped_output_path();
                }
                state.status = "Output path set to " + state.output_path;
                window.request_redraw();
            }

            if (just_clicked && timeline_hit) {
                const std::size_t total_samples = recorder.captured_samples();
                const int span = std::max(1, timeline_rect.right - timeline_rect.left);
                const int rel_x = std::clamp(pointer.x - timeline_rect.left, 0, span);
                const double t = static_cast<double>(rel_x) / static_cast<double>(span);

                const auto [view_start, view_end] = timeline_view_bounds(total_samples);
                const std::size_t view_span_samples = std::max<std::size_t>(1, view_end - view_start);
                const std::size_t local_sample = static_cast<std::size_t>(t * static_cast<double>(view_span_samples));
                state.playhead_sample = std::min(total_samples, view_start + local_sample);
                if (!state.timeline_follow_playhead) {
                    state.timeline_view_center_sample = state.playhead_sample;
                }
                state.status = "Playhead moved to " + format_seconds(static_cast<double>(state.playhead_sample) /
                                                                      static_cast<double>(std::max(1u, recorder.sample_rate())));
                window.request_redraw();
            }

            if (just_clicked && mode_segment_hit_index == 0) {
                state.timeline_follow_playhead = true;
                state.timeline_view_center_sample = state.playhead_sample;
                state.status = "Timeline mode set to Follow.";
                window.request_redraw();
            }

            if (just_clicked && mode_segment_hit_index == 1) {
                state.timeline_follow_playhead = false;
                state.timeline_view_center_sample = state.playhead_sample;
                state.status = "Timeline mode set to Manual Pan.";
                window.request_redraw();
            }

            if (just_clicked && overview_hit) {
                state.overview_drag_active = true;
                update_center_from_overview_x(pointer.x);
                state.status = "Overview dragged.";
                window.request_redraw();
            }

            if (pointer.left_button_down && state.overview_drag_active) {
                update_center_from_overview_x(pointer.x);
                window.request_redraw();
            }

            if (!pointer.left_button_down) {
                state.overview_drag_active = false;
            }

            if (just_clicked && pan_left_hit) {
                const std::size_t total_samples = recorder.captured_samples();
                const auto [view_start, view_end] = timeline_view_bounds(total_samples);
                const std::size_t step = std::max<std::size_t>(1, (view_end - view_start) / 4);
                const std::size_t current_center = (view_start + view_end) / 2;
                state.timeline_follow_playhead = false;
                state.timeline_view_center_sample = (current_center > step) ? (current_center - step) : 0;
                state.status = "Timeline panned left.";
                window.request_redraw();
            }

            if (just_clicked && pan_right_hit) {
                const std::size_t total_samples = recorder.captured_samples();
                const auto [view_start, view_end] = timeline_view_bounds(total_samples);
                const std::size_t step = std::max<std::size_t>(1, (view_end - view_start) / 4);
                const std::size_t current_center = (view_start + view_end) / 2;
                state.timeline_follow_playhead = false;
                state.timeline_view_center_sample = std::min(total_samples, current_center + step);
                state.status = "Timeline panned right.";
                window.request_redraw();
            }

            if (just_clicked && zoom_out_hit) {
                state.timeline_zoom = std::max(1, state.timeline_zoom / 2);
                state.status = "Timeline zoom set to " + std::to_string(state.timeline_zoom) + "x";
                window.request_redraw();
            }

            if (just_clicked && zoom_in_hit) {
                state.timeline_zoom = std::min(64, state.timeline_zoom * 2);
                state.status = "Timeline zoom set to " + std::to_string(state.timeline_zoom) + "x";
                window.request_redraw();
            }

            if (just_clicked && mark_a_hit) {
                state.has_mark_a = true;
                state.mark_a_sample = state.playhead_sample;
                state.status = "Mark A set.";
                window.request_redraw();
            }

            if (just_clicked && mark_b_hit) {
                state.has_mark_b = true;
                state.mark_b_sample = state.playhead_sample;
                state.status = "Mark B set.";
                window.request_redraw();
            }

            if (just_clicked && cut_hit) {
                if (!state.has_mark_a || !state.has_mark_b || state.mark_a_sample == state.mark_b_sample) {
                    state.status = "Set distinct Mark A and Mark B before cutting.";
                } else {
                    const std::size_t start = std::min(state.mark_a_sample, state.mark_b_sample);
                    const std::size_t end = std::max(state.mark_a_sample, state.mark_b_sample);
                    if (recorder.cut_range(start, end)) {
                        state.playhead_sample = start;
                        state.mark_a_sample = start;
                        state.mark_b_sample = start;
                        state.playhead_sample = std::min(state.playhead_sample, recorder.captured_samples());
                        state.status = "Cut applied to selected range.";
                    } else {
                        state.status = "Cut failed for selected range.";
                    }
                }
                window.request_redraw();
            }

            if (just_clicked && !state.recording && !microphones.empty()) {
                for (std::size_t i = 0; i < microphones.size(); ++i) {
                    const auto [row_top, row_bottom] = mic_row_bounds(i);
                    const bool mic_hit = pointer.inside &&
                                         pointer.x >= mic_row_bounds_rect.left &&
                                         pointer.x <= mic_row_bounds_rect.right &&
                                         pointer.y >= row_top &&
                                         pointer.y <= row_bottom;
                    if (mic_hit) {
                        state.selected_microphone_index = static_cast<int>(i);
                        state.selected_microphone = microphones[i].name;
                        state.status = "Selected microphone: " + microphones[i].name;
                        window.request_redraw();
                        break;
                    }
                }
            }

            previous_left_down = pointer.left_button_down;
        }

        window.request_redraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (recorder.is_recording()) {
        std::string error;
        if (!recorder.stop(error)) {
            std::cerr << "Warning: could not stop recording cleanly: " << error << "\n";
        }
    }

    return 0;
}

} // namespace

int main(int argc, const char* const argv[]) {
    const Options options = parse_args(argc, argv);
    if (options.cli_mode) {
        return run_cli(options);
    }
    return run_window(options);
}
