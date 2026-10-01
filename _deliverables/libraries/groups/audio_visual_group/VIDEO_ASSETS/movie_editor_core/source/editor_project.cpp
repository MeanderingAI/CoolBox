#include "../headers/movie_editor_core.h"

#include <stdexcept>

namespace trekker {
namespace movie_editor {

timeline::Track& EditorProject::add_video_track(const std::string& name) {
    return timeline_.add_track(timeline::TrackType::Video, name);
}

timeline::Track& EditorProject::add_audio_track(const std::string& name) {
    return timeline_.add_track(timeline::TrackType::Audio, name);
}

std::uint64_t EditorProject::add_clip(std::uint64_t track_id, const std::string& media_key,
                                      std::int64_t position_us,
                                      std::int64_t in_us, std::int64_t out_us) {
    timeline::Track* track = timeline_.track(track_id);
    if (!track) throw std::invalid_argument("add_clip: unknown track id");

    const MediaAsset* asset = bin_.find(media_key);
    if (!asset) throw std::invalid_argument("add_clip: unknown media key: " + media_key);

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
    return timeline_.split_clip(track_id, pts_us);
}

bool EditorProject::delete_clip(std::uint64_t track_id, std::uint64_t clip_id) {
    timeline::Track* track = timeline_.track(track_id);
    if (!track) return false;
    return track->remove_clip(clip_id);
}

} // namespace movie_editor
} // namespace trekker
