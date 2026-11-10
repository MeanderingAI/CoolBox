#include "../headers/movie_editor_core.h"

#include <stdexcept>
#include <cmath>
#include <limits>

namespace trekker {
namespace movie_editor {

void ensure_text_font_available();

namespace {
void validate_clip_range(const MediaAsset& asset, std::int64_t position_us,
                         std::int64_t in_us, std::int64_t end) {
    const bool bounded = asset.type == MediaType::Audio || asset.type == MediaType::ImportedVideo;
    if (position_us < 0 || in_us < 0 || end <= in_us ||
        (bounded && end > asset.duration_us) ||
        position_us > std::numeric_limits<std::int64_t>::max() - (end - in_us))
        throw std::invalid_argument("add_clip: invalid source range or position");
}
}

void validate_text_style(const TextStyle& style) {
    if (style.text.empty() || style.text.size() > 16384 ||
        style.color > 0xffffff || style.background_color > 0xffffff ||
        !std::isfinite(style.background_opacity) || style.background_opacity < 0 || style.background_opacity > 1 ||
        !std::isfinite(style.opacity) || style.opacity < 0 || style.opacity > 1 ||
        !std::isfinite(style.font_size) || style.font_size < 1 || style.font_size > 720 ||
        !std::isfinite(style.x) || !std::isfinite(style.y) ||
        style.x < 0 || style.x > 1 || style.y < 0 || style.y > 1 ||
        style.fade_in_us < 0 || style.fade_out_us < 0 ||
        style.text.find('\0') != std::string::npos)
        throw std::invalid_argument("Invalid text, RGB colors, font size, normalized position, or fades.");
}

std::uint64_t EditorProject::add_text_clip(const TextStyle& style, std::int64_t position_us,
                                          std::int64_t duration_us) {
    validate_text_style(style);
    if (position_us < 0 || duration_us <= 0 ||
        position_us > std::numeric_limits<std::int64_t>::max() - duration_us)
        throw std::invalid_argument("Text clip requires a positive duration and valid position.");
    ensure_text_font_available();
    timeline::Clip clip;
    clip.id = next_clip_id_++;
    clip.text = style;
    clip.position_us = position_us;
    clip.out_point_us = duration_us;
    checkpoint();
    timeline_.add_track(timeline::TrackType::Video, style.background ? "Title" : "Text overlay").add_clip(clip);
    return clip.id;
}

const TextStyle* EditorProject::text_style(std::uint64_t track_id, std::uint64_t clip_id) const {
    const auto* track = timeline_.track(track_id);
    if (track) for (const auto& clip : track->clips())
        if (clip.id == clip_id && clip.text) return &*clip.text;
    return nullptr;
}

void EditorProject::edit_text_clip(std::uint64_t track_id, std::uint64_t clip_id,
                                   const TextStyle& style, std::int64_t duration_us) {
    validate_text_style(style);
    auto* track = timeline_.track(track_id);
    if (!track || track->locked()) throw std::invalid_argument("Text editing requires an unlocked track.");
    const auto found = std::find_if(track->clips().begin(), track->clips().end(),
        [clip_id](const auto& clip) { return clip.id == clip_id; });
    if (found == track->clips().end() || !found->text)
        throw std::invalid_argument("Text clip not found.");
    if (duration_us <= 0 || found->position_us > std::numeric_limits<std::int64_t>::max() - duration_us)
        throw std::invalid_argument("Invalid text clip duration.");
    ensure_text_font_available();
    auto clip = *found;
    clip.text = style;
    clip.in_point_us = 0;
    clip.out_point_us = duration_us;
    clip.speed = 1;
    checkpoint();
    track->remove_clip(clip_id);
    track->add_clip(std::move(clip));
}

timeline::Track& EditorProject::add_video_track(const std::string& name) {
    checkpoint();
    return timeline_.add_track(timeline::TrackType::Video, name);
}

timeline::Track& EditorProject::add_audio_track(const std::string& name) {
    checkpoint();
    return timeline_.add_track(timeline::TrackType::Audio, name);
}

bool EditorProject::delete_track(std::uint64_t track_id) {
    const auto* track = timeline_.track(track_id);
    if (!track || track->locked()) return false;
    checkpoint();
    return timeline_.remove_track(track_id);
}

std::uint64_t EditorProject::add_clip_on_new_track(const std::string& media_key, std::int64_t position_us) {
    const auto* asset = bin_.find(media_key);
    if (!asset) throw std::invalid_argument("add_clip: unknown media key: " + media_key);
    validate_clip_range(*asset, position_us, 0, asset->duration_us);
    timeline::Clip clip;
    clip.id = next_clip_id_++;
    clip.source_path = media_key;
    clip.position_us = position_us;
    clip.out_point_us = asset->duration_us;
    checkpoint();
    const bool audio = asset->type == MediaType::Audio;
    auto& track = timeline_.add_track(audio ? timeline::TrackType::Audio : timeline::TrackType::Video,
                                     audio ? "Audio" : "Video");
    track.add_clip(clip);
    return clip.id;
}

std::uint64_t EditorProject::add_clip(std::uint64_t track_id, const std::string& media_key,
                                      std::int64_t position_us,
                                      std::int64_t in_us, std::int64_t out_us) {
    timeline::Track* track = timeline_.track(track_id);
    if (!track) throw std::invalid_argument("add_clip: unknown track id");

    const MediaAsset* asset = bin_.find(media_key);
    if (!asset) throw std::invalid_argument("add_clip: unknown media key: " + media_key);
    if (track->locked()) throw std::invalid_argument("add_clip: track is locked");
    if ((asset->type == MediaType::Audio) != (track->type() == timeline::TrackType::Audio))
        throw std::invalid_argument("add_clip: incompatible track type");
    const auto end = out_us > 0 ? out_us : asset->duration_us;
    validate_clip_range(*asset, position_us, in_us, end);
    checkpoint();

    timeline::Clip clip;
    clip.id = next_clip_id_++;
    clip.source_path = media_key;
    clip.in_point_us = in_us;
    clip.out_point_us = out_us > 0 ? out_us : asset->duration_us;
    clip.position_us = position_us;

    track->add_clip(clip);
    return clip.id;
}

bool EditorProject::split_clip(std::uint64_t track_id, std::int64_t pts_us) {
    auto* track = timeline_.track(track_id);
    if (!track || track->locked()) return false;
    const auto* existing = track->clip_at(pts_us);
    if (!existing || pts_us <= existing->position_us) return false;
    auto left = *existing;
    auto right = left;
    left.out_point_us = left.in_point_us +
        static_cast<std::int64_t>(static_cast<long double>(pts_us - left.position_us) * left.speed);
    if (left.out_point_us <= left.in_point_us || left.out_point_us >= right.out_point_us) return false;
    checkpoint();
    left.id = next_clip_id_++;
    right.id = next_clip_id_++;
    right.in_point_us = left.out_point_us;
    right.position_us = pts_us;
    track->remove_clip(existing->id);
    track->add_clip(std::move(left));
    track->add_clip(std::move(right));
    return true;
}

bool EditorProject::delete_clip(std::uint64_t track_id, std::uint64_t clip_id) {
    timeline::Track* track = timeline_.track(track_id);
    if (!track || track->locked()) return false;
    const auto found = std::find_if(track->clips().begin(), track->clips().end(),
        [clip_id](const auto& clip) { return clip.id == clip_id; });
    if (found == track->clips().end()) return false;
    checkpoint();
    return track->remove_clip(clip_id);
}

void EditorProject::checkpoint() {
    if (undo_.size() == 100) undo_.erase(undo_.begin());
    undo_.push_back(timeline_);
    redo_.clear();
}

bool EditorProject::undo() {
    if (undo_.empty()) return false;
    redo_.push_back(timeline_);
    timeline_ = std::move(undo_.back());
    undo_.pop_back();
    return true;
}

bool EditorProject::redo() {
    if (redo_.empty()) return false;
    undo_.push_back(timeline_);
    timeline_ = std::move(redo_.back());
    redo_.pop_back();
    return true;
}

void EditorProject::trim_clip(std::uint64_t track_id, std::uint64_t clip_id,
                             std::int64_t in_us, std::int64_t out_us, std::int64_t position_us) {
    auto* track = timeline_.track(track_id);
    if (!track || track->locked()) throw std::invalid_argument("Trim requires an unlocked track.");
    const auto found = std::find_if(track->clips().begin(), track->clips().end(),
        [clip_id](const auto& clip) { return clip.id == clip_id; });
    if (found == track->clips().end()) throw std::invalid_argument("Clip not found.");
    const auto* asset = bin_.find(found->source_path);
    if ((!asset && !found->text) || position_us < 0 || in_us < 0 || out_us <= in_us ||
        (!found->text && out_us > asset->duration_us) || !std::isfinite(found->speed) || found->speed <= 0 ||
        (out_us - in_us) / found->speed < 1 ||
        static_cast<long double>(out_us - in_us) / found->speed >
            std::numeric_limits<std::int64_t>::max() - position_us)
        throw std::invalid_argument("Trim exceeds the media bounds or creates an empty clip.");
    if (found->in_point_us == in_us && found->out_point_us == out_us && found->position_us == position_us) return;
    auto clip = *found;
    clip.in_point_us = in_us;
    clip.out_point_us = out_us;
    clip.position_us = position_us;
    checkpoint();
    track->remove_clip(clip_id);
    track->add_clip(std::move(clip));
}

void EditorProject::move_clip(std::uint64_t from_track, std::uint64_t clip_id,
                             std::uint64_t to_track, std::int64_t position_us) {
    auto* source = timeline_.track(from_track);
    auto* destination = timeline_.track(to_track);
    if (!source || !destination || source->locked() || destination->locked())
        throw std::invalid_argument("Move requires unlocked source and destination tracks.");
    if (source->type() != destination->type() || position_us < 0)
        throw std::invalid_argument("Choose a matching track and a non-negative position.");
    const auto found = std::find_if(source->clips().begin(), source->clips().end(),
        [clip_id](const auto& clip) { return clip.id == clip_id; });
    if (found == source->clips().end()) throw std::invalid_argument("Clip not found.");
    if (!std::isfinite(found->speed) || found->speed <= 0 ||
        static_cast<long double>(found->out_point_us - found->in_point_us) / found->speed >
            std::numeric_limits<std::int64_t>::max() - position_us)
        throw std::invalid_argument("Clip duration exceeds the timeline range.");
    if (from_track == to_track && found->position_us == position_us) return;
    auto clip = *found;
    clip.position_us = position_us;
    checkpoint();
    source->remove_clip(clip_id);
    destination->add_clip(std::move(clip));
}

} // namespace movie_editor
} // namespace trekker
