#include "../headers/movie_editor_core.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace trekker {
namespace movie_editor {

namespace fs = std::filesystem;

namespace {

bool is_recognised_image(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

} // namespace

std::string MediaBin::import_image_sequence(const std::string& directory, double fps) {
    if (fps <= 0.0) throw std::invalid_argument("import_image_sequence: fps must be positive");

    std::vector<std::string> frames;
    if (!fs::is_directory(directory)) {
        throw std::runtime_error("import_image_sequence: not a directory: " + directory);
    }
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && is_recognised_image(entry.path())) {
            frames.push_back(fs::absolute(entry.path()).string());
        }
    }
    if (frames.empty()) {
        throw std::runtime_error("import_image_sequence: no recognised image files in " + directory);
    }
    std::sort(frames.begin(), frames.end());

    MediaAsset asset;
    asset.type = MediaType::ImageSequence;
    asset.path = directory;
    asset.fps = fps;
    asset.frame_files = std::move(frames);
    asset.duration_us = static_cast<std::int64_t>(
        (static_cast<double>(asset.frame_files.size()) / fps) * 1'000'000.0);

    const auto it = index_by_path_.find(asset.path);
    if (it != index_by_path_.end()) {
        assets_[it->second] = std::move(asset);
        return assets_[it->second].path;
    }
    index_by_path_[asset.path] = assets_.size();
    assets_.push_back(std::move(asset));
    return assets_.back().path;
}

std::string MediaBin::import_still_image(const std::string& image_path, std::int64_t duration_us) {
    if (!fs::is_regular_file(image_path)) {
        throw std::runtime_error("import_still_image: file not found: " + image_path);
    }
    MediaAsset asset;
    asset.type = MediaType::StillImage;
    asset.path = fs::absolute(image_path).string();
    asset.frame_files = {asset.path};
    asset.duration_us = duration_us;

    const auto it = index_by_path_.find(asset.path);
    if (it != index_by_path_.end()) {
        assets_[it->second] = std::move(asset);
        return assets_[it->second].path;
    }
    index_by_path_[asset.path] = assets_.size();
    assets_.push_back(std::move(asset));
    return assets_.back().path;
}

std::string MediaBin::import_audio(const std::string& wav_path) {
    const audio::AudioBuffer buf = audio::container::read_wav(wav_path);

    MediaAsset asset;
    asset.type = MediaType::Audio;
    asset.path = fs::absolute(wav_path).string();
    asset.audio_sample_rate = buf.sample_rate;
    asset.audio_channels = buf.num_channels;
    asset.duration_us = static_cast<std::int64_t>(buf.duration_us());

    const auto it = index_by_path_.find(asset.path);
    if (it != index_by_path_.end()) {
        assets_[it->second] = std::move(asset);
        return assets_[it->second].path;
    }
    index_by_path_[asset.path] = assets_.size();
    assets_.push_back(std::move(asset));
    return assets_.back().path;
}

std::string MediaBin::import_video_file(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (!fs::is_regular_file(path)) {
        throw std::runtime_error("import_video_file: file not found: " + path);
    }

    MediaAsset asset;
    asset.type = MediaType::ImportedVideo;
    asset.path = fs::absolute(path).string();

    if (ext == ".gif") {
        video::container::DecodedGif decoded = video::container::read_gif(path);
        asset.decoded_frames = std::move(decoded.frames);
        asset.frame_start_us.reserve(decoded.frame_delay_us.size());
        std::int64_t cursor_us = 0;
        for (std::int64_t delay_us : decoded.frame_delay_us) {
            asset.frame_start_us.push_back(cursor_us);
            cursor_us += delay_us;
        }
        asset.duration_us = cursor_us;
        asset.fps = asset.decoded_frames.empty() || asset.duration_us <= 0
            ? 24.0
            : (static_cast<double>(asset.decoded_frames.size()) * 1'000'000.0) / static_cast<double>(asset.duration_us);
    } else if (ext == ".avi") {
        video::container::DecodedAvi decoded = video::container::read_avi(path);
        asset.decoded_frames = std::move(decoded.frames);
        asset.fps = decoded.fps > 0.0 ? decoded.fps : 24.0;
        const std::int64_t frame_duration_us = static_cast<std::int64_t>(1'000'000.0 / asset.fps);
        asset.frame_start_us.reserve(asset.decoded_frames.size());
        for (std::size_t i = 0; i < asset.decoded_frames.size(); ++i) {
            asset.frame_start_us.push_back(static_cast<std::int64_t>(i) * frame_duration_us);
        }
        asset.duration_us = static_cast<std::int64_t>(asset.decoded_frames.size()) * frame_duration_us;
    } else {
        throw std::runtime_error("import_video_file: unsupported video format '" + ext +
                                 "' — only .gif and .avi can be decoded (formats like .mp4/.mov "
                                 "require H.264/H.265 codecs this from-scratch library does not implement)");
    }

    const auto it = index_by_path_.find(asset.path);
    if (it != index_by_path_.end()) {
        assets_[it->second] = std::move(asset);
        return assets_[it->second].path;
    }
    index_by_path_[asset.path] = assets_.size();
    assets_.push_back(std::move(asset));
    return assets_.back().path;
}


const MediaAsset* MediaBin::find(const std::string& key) const {
    const auto it = index_by_path_.find(key);
    if (it == index_by_path_.end()) return nullptr;
    return &assets_[it->second];
}

} // namespace movie_editor
} // namespace trekker
