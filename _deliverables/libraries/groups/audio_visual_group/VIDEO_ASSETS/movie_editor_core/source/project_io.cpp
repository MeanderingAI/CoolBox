#include "../headers/movie_editor_core.h"
#include "json.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace trekker {
namespace movie_editor {

namespace {

using dataformats::json::Array;
using dataformats::json::Object;
using dataformats::json::Value;

const char* media_type_name(MediaType t) {
    switch (t) {
        case MediaType::ImageSequence: return "image_sequence";
        case MediaType::StillImage:    return "still_image";
        case MediaType::Audio:         return "audio";
    }
    return "still_image";
}

MediaType media_type_from_name(const std::string& name) {
    if (name == "image_sequence") return MediaType::ImageSequence;
    if (name == "audio") return MediaType::Audio;
    return MediaType::StillImage;
}

const char* track_type_name(timeline::TrackType t) {
    return t == timeline::TrackType::Video ? "video" : "audio";
}

timeline::TrackType track_type_from_name(const std::string& name) {
    return name == "audio" ? timeline::TrackType::Audio : timeline::TrackType::Video;
}

} // namespace

void EditorProject::save_project(const std::string& path) const {
    Object root;

    Array media_array;
    for (const auto& asset : bin_.assets()) {
        Object m;
        m.set("key", Value(asset.path));
        m.set("type", Value(media_type_name(asset.type)));
        m.set("fps", Value(asset.fps));
        m.set("duration_us", Value(static_cast<double>(asset.duration_us)));
        if (asset.type == MediaType::StillImage) {
            m.set("image_path", Value(asset.frame_files.empty() ? std::string() : asset.frame_files.front()));
        }
        media_array.push(Value(m));
    }
    root.set("media", Value(media_array));

    Array tracks_array;
    for (const auto& track : timeline_.tracks()) {
        Object t;
        t.set("id", Value(static_cast<double>(track.id())));
        t.set("type", Value(track_type_name(track.type())));
        t.set("name", Value(track.name()));
        t.set("muted", Value(track.muted()));
        t.set("locked", Value(track.locked()));

        Array clips_array;
        for (const auto& clip : track.clips()) {
            Object c;
            c.set("id", Value(static_cast<double>(clip.id)));
            c.set("source_path", Value(clip.source_path));
            c.set("in_point_us", Value(static_cast<double>(clip.in_point_us)));
            c.set("out_point_us", Value(static_cast<double>(clip.out_point_us)));
            c.set("position_us", Value(static_cast<double>(clip.position_us)));
            c.set("speed", Value(static_cast<double>(clip.speed)));
            c.set("volume", Value(static_cast<double>(clip.volume)));
            c.set("muted", Value(clip.muted));
            clips_array.push(Value(c));
        }
        t.set("clips", Value(clips_array));
        tracks_array.push(Value(t));
    }
    root.set("tracks", Value(tracks_array));

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("save_project: failed to open output path: " + path);
    out << Value(root).to_string();
}

void EditorProject::load_project(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("load_project: failed to open input path: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();

    const Value root = dataformats::json::Parser::parse(ss.str());
    if (!root.is_object()) throw std::runtime_error("load_project: root is not a JSON object: " + path);
    const Object root_obj = root.as_object();

    MediaBin fresh_bin;
    if (root_obj.has("media")) {
        for (const auto& media_value : root_obj.get("media").as_array()) {
            const Object m = media_value.as_object();
            const std::string key = m.get("key").as_string();
            const MediaType type = media_type_from_name(m.get("type").as_string());
            if (type == MediaType::ImageSequence) {
                const double fps = m.has("fps") ? m.get("fps").as_number() : 24.0;
                fresh_bin.import_image_sequence(key, fps);
            } else if (type == MediaType::Audio) {
                fresh_bin.import_audio(key);
            } else {
                const std::string image_path = m.has("image_path") ? m.get("image_path").as_string() : key;
                const std::int64_t duration_us = m.has("duration_us")
                    ? static_cast<std::int64_t>(m.get("duration_us").as_number()) : 5'000'000;
                fresh_bin.import_still_image(image_path, duration_us);
            }
        }
    }

    timeline::Timeline fresh_timeline;
    std::uint64_t max_clip_id = 0;
    if (root_obj.has("tracks")) {
        for (const auto& track_value : root_obj.get("tracks").as_array()) {
            const Object t = track_value.as_object();
            timeline::Track& track = fresh_timeline.add_track(
                track_type_from_name(t.get("type").as_string()),
                t.has("name") ? t.get("name").as_string() : std::string());
            track.set_muted(t.has("muted") && t.get("muted").as_bool());
            track.set_locked(t.has("locked") && t.get("locked").as_bool());

            if (t.has("clips")) {
                for (const auto& clip_value : t.get("clips").as_array()) {
                    const Object c = clip_value.as_object();
                    timeline::Clip clip;
                    clip.id = static_cast<std::uint64_t>(c.get("id").as_number());
                    clip.source_path = c.get("source_path").as_string();
                    clip.in_point_us = static_cast<std::int64_t>(c.get("in_point_us").as_number());
                    clip.out_point_us = static_cast<std::int64_t>(c.get("out_point_us").as_number());
                    clip.position_us = static_cast<std::int64_t>(c.get("position_us").as_number());
                    clip.speed = c.has("speed") ? static_cast<float>(c.get("speed").as_number()) : 1.0f;
                    clip.volume = c.has("volume") ? static_cast<float>(c.get("volume").as_number()) : 1.0f;
                    clip.muted = c.has("muted") && c.get("muted").as_bool();
                    max_clip_id = std::max(max_clip_id, clip.id);
                    track.add_clip(clip);
                }
            }
        }
    }

    bin_ = std::move(fresh_bin);
    timeline_ = std::move(fresh_timeline);
    audio_cache_.clear();
    frame_cache_.clear();
    next_clip_id_ = std::max<std::uint64_t>(1'000'000, max_clip_id + 1);
}

} // namespace movie_editor
} // namespace trekker
