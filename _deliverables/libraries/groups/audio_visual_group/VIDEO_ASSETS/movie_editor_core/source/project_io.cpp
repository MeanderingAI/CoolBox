#include "../headers/movie_editor_core.h"
#include "json.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <iomanip>
#include <locale>

namespace trekker {
namespace movie_editor {

void ensure_text_font_available();

namespace {

using dataformats::json::Array;
using dataformats::json::Object;
using dataformats::json::Value;

// Value::to_string uses stream-default six-digit precision for numbers,
// which changes RGB colors, microsecond timestamps, and normalized anchors.
void write_project_json(std::ostream& out, const Value& value) {
    if (value.is_number()) {
        out << std::setprecision(std::numeric_limits<double>::max_digits10) << value.as_number();
    } else if (value.is_object()) {
        out << '{';
        bool first = true;
        for (const auto& entry : value.as_object()) {
            if (!first) out << ',';
            first = false;
            out << Value(entry.first).to_string() << ':';
            write_project_json(out, entry.second);
        }
        out << '}';
    } else if (value.is_array()) {
        out << '[';
        bool first = true;
        for (const auto& entry : value.as_array()) {
            if (!first) out << ',';
            first = false;
            write_project_json(out, entry);
        }
        out << ']';
    } else {
        out << value.to_string();
    }
}

std::int64_t text_integer(const Object& object, const char* key, std::int64_t fallback) {
    if (!object.has(key)) return fallback;
    const double number = object.get(key).as_number();
    if (!std::isfinite(number) || number < 0 ||
        static_cast<long double>(number) > std::numeric_limits<std::int64_t>::max() ||
        std::floor(number) != number)
        throw std::invalid_argument(std::string("Invalid text integer: ") + key);
    return static_cast<std::int64_t>(number);
}

const char* media_type_name(MediaType t) {
    switch (t) {
        case MediaType::ImageSequence: return "image_sequence";
        case MediaType::StillImage:    return "still_image";
        case MediaType::Audio:         return "audio";
        case MediaType::ImportedVideo: return "imported_video";
    }
    return "still_image";
}

MediaType media_type_from_name(const std::string& name) {
    if (name == "image_sequence") return MediaType::ImageSequence;
    if (name == "audio") return MediaType::Audio;
    if (name == "imported_video") return MediaType::ImportedVideo;
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
            if (clip.text) {
                const auto& style = *clip.text;
                Object text;
                text.set("text", Value(style.text));
                text.set("background", Value(style.background));
                text.set("background_opacity", Value(style.background_opacity));
                text.set("opacity", Value(style.opacity));
                text.set("color", Value(static_cast<double>(style.color)));
                text.set("background_color", Value(static_cast<double>(style.background_color)));
                text.set("font_size", Value(style.font_size));
                text.set("x", Value(style.x));
                text.set("y", Value(style.y));
                text.set("fade_in_us", Value(static_cast<double>(style.fade_in_us)));
                text.set("fade_out_us", Value(static_cast<double>(style.fade_out_us)));
                c.set("text", Value(text));
            }
            clips_array.push(Value(c));
        }
        t.set("clips", Value(clips_array));
        tracks_array.push(Value(t));
    }
    root.set("tracks", Value(tracks_array));

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("save_project: failed to open output path: " + path);
    out.imbue(std::locale::classic());
    write_project_json(out, Value(root));
    out.close();
    if (!out) throw std::runtime_error("save_project: failed to finish writing: " + path);
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
            } else if (type == MediaType::ImportedVideo) {
                // decoded_frames are never serialized (they're in-memory
                // only) — re-decode from the original .gif/.avi path instead.
                fresh_bin.import_video_file(key);
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
                    if (c.has("text")) {
                        const auto text = c.get("text").as_object();
                        TextStyle style;
                        if (text.has("text")) style.text = text.get("text").as_string();
                        if (text.has("background")) style.background = text.get("background").as_bool();
                        if (text.has("background_opacity")) style.background_opacity = text.get("background_opacity").as_number();
                        if (text.has("opacity")) style.opacity = text.get("opacity").as_number();
                        const auto color = text_integer(text, "color", style.color);
                        const auto background = text_integer(text, "background_color", style.background_color);
                        if (color > 0xffffff || background > 0xffffff)
                            throw std::invalid_argument("Text colors must be 24-bit RGB.");
                        style.color = static_cast<std::uint32_t>(color);
                        style.background_color = static_cast<std::uint32_t>(background);
                        if (text.has("font_size")) style.font_size = text.get("font_size").as_number();
                        if (text.has("x")) style.x = text.get("x").as_number();
                        if (text.has("y")) style.y = text.get("y").as_number();
                        style.fade_in_us = text_integer(text, "fade_in_us", style.fade_in_us);
                        style.fade_out_us = text_integer(text, "fade_out_us", style.fade_out_us);
                        validate_text_style(style);
                        ensure_text_font_available();
                        clip.in_point_us = text_integer(c, "in_point_us", 0);
                        clip.out_point_us = text_integer(c, "out_point_us", 0);
                        clip.position_us = text_integer(c, "position_us", 0);
                        if (track.type() != timeline::TrackType::Video ||
                            clip.out_point_us <= clip.in_point_us ||
                            !std::isfinite(clip.speed) || clip.speed <= 0 ||
                            (clip.out_point_us - clip.in_point_us) / clip.speed < 1 ||
                            static_cast<long double>(clip.out_point_us - clip.in_point_us) / clip.speed >
                                std::numeric_limits<std::int64_t>::max() - clip.position_us)
                            throw std::invalid_argument("Invalid text clip range or track.");
                        clip.text = std::move(style);
                    }
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
    undo_.clear();
    redo_.clear();
}

} // namespace movie_editor
} // namespace trekker
