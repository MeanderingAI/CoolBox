#ifndef TREKKER_VIDEO_ASSETS_MOVIE_EDITOR_CORE_H
#define TREKKER_VIDEO_ASSETS_MOVIE_EDITOR_CORE_H

// movie_editor_core — the non-linear-editing engine behind the movie_editor
// app: a media bin (imported image-sequence/still/audio assets), a project
// built on trekker::timeline::Timeline, frame/audio compositing, and export
// to the video_container/audio_container encoders.
//
// This library is intentionally UI-agnostic (no windowing, no drawing) so it
// can be unit-tested directly; movie_editor's GUI/CLI layers are thin
// wrappers around it.

#include "../../audio_container/headers/audio_container.h"
#include "../../audio_processing/headers/audio_processing.h"
#include "../../video_codec/headers/video_codec.h"
#include "../../video_container/headers/video_container.h"
#include "../../video_timeline/headers/video_timeline.h"
#include "graphics/texture_loader.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace trekker {
namespace movie_editor {

// ── Media bin ─────────────────────────────────────────────────────────────────

enum class MediaType { ImageSequence, StillImage, Audio, ImportedVideo };

struct MediaAsset {
    MediaType type = MediaType::StillImage;
    std::string path;                      // directory (ImageSequence) or file path
    double fps = 24.0;                     // ImageSequence only
    std::vector<std::string> frame_files;  // sorted absolute frame paths (size 1 for stills)
    std::int64_t duration_us = 0;
    std::uint32_t audio_sample_rate = 0;   // Audio only
    std::uint32_t audio_channels = 0;      // Audio only

    // ImportedVideo only (decoded GIF/AVI frames, held in memory rather than
    // as files on disk since the source container doesn't expose individual
    // frames as separate image files).
    std::vector<video::VideoFrame> decoded_frames;
    std::vector<std::int64_t> frame_start_us; // same length as decoded_frames; cumulative
};

class MediaBin {
public:
    // Imports every recognised image file (.png/.jpg/.jpeg/.bmp) in
    // `directory`, sorted lexicographically, as a single video asset played
    // back at `fps`. Returns the asset's key (the directory path) for use
    // with EditorProject::add_clip(). Throws std::runtime_error if the
    // directory contains no recognised image files.
    std::string import_image_sequence(const std::string& directory, double fps);

    // Imports a single still image, held for `duration_us` (default 5s).
    std::string import_still_image(const std::string& image_path,
                                          std::int64_t duration_us = 5'000'000);

    // Imports a WAV file (PCM 8/16/32-bit, IEEE float, or IMA ADPCM).
    std::string import_audio(const std::string& wav_path);

    // Imports a .gif or .avi file (the only "real video file" formats this
    // from-scratch codebase can decode — see video_container's read_gif/
    // read_avi) as a single video asset. AVI's own embedded audio track (if
    // any) is decoded but not auto-added to the bin as a separate asset;
    // import it again via import_audio() from an extracted WAV if needed.
    // Throws std::runtime_error for unrecognised extensions (notably
    // .mp4/.mov, which require codecs this library does not implement) or
    // malformed files.
    std::string import_video_file(const std::string& path);

    const MediaAsset* find(const std::string& key) const;
    const std::vector<MediaAsset>& assets() const { return assets_; }

private:
    std::vector<MediaAsset> assets_;
    std::unordered_map<std::string, std::size_t> index_by_path_;
};

// ── Editor project ────────────────────────────────────────────────────────────

class EditorProject {
public:
    EditorProject() = default;

    MediaBin& media_bin() { return bin_; }
    const MediaBin& media_bin() const { return bin_; }
    timeline::Timeline& timeline() { return timeline_; }
    const timeline::Timeline& timeline() const { return timeline_; }

    timeline::Track& add_video_track(const std::string& name = {});
    timeline::Track& add_audio_track(const std::string& name = {});

    // Adds a clip referencing an already-imported media asset (keyed by its
    // MediaBin path/directory) to `track_id` at `position_us`. in_us/out_us
    // trim the source; out_us == 0 means "use the asset's full duration".
    // Throws std::invalid_argument if the track or media key is unknown.
    std::uint64_t add_clip(std::uint64_t track_id, const std::string& media_key,
                           std::int64_t position_us,
                           std::int64_t in_us = 0, std::int64_t out_us = 0);

    bool split_clip(std::uint64_t track_id, std::int64_t pts_us);
    bool delete_clip(std::uint64_t track_id, std::uint64_t clip_id);

    std::int64_t duration_us() const { return timeline_.duration_us(); }

    // Renders the composited frame visible at `pts_us` at (out_width x
    // out_height): the topmost (highest track index) unmuted video track
    // with a clip covering pts_us wins (simple front-to-back priority, no
    // alpha blending between tracks). Returns a solid black RGB24 frame if
    // no video clip covers pts_us.
    video::VideoFrame render_frame_at(std::int64_t pts_us, std::size_t out_width, std::size_t out_height) const;

    // Mixes every unmuted audio clip overlapping [start_us, end_us) down to
    // one AudioBuffer at (sample_rate, channels), applying per-clip volume
    // and resampling as needed. Clip playback speed is not applied to audio
    // (time-stretching audio is out of scope); the clip's source samples are
    // used at their native rate/pitch.
    audio::AudioBuffer render_audio_mix(std::int64_t start_us, std::int64_t end_us,
                                        std::uint32_t sample_rate, std::uint32_t channels) const;

    // ── Export ──────────────────────────────────────────────────────────────

    // Renders the full timeline (video composited per render_frame_at() plus
    // the mixed audio) to a standard AVI file.
    void export_avi(const std::string& path, double fps, std::size_t width, std::size_t height,
                    std::uint32_t audio_sample_rate = 48000, std::uint32_t audio_channels = 2) const;

    // Renders [start_us, end_us) as an animated GIF preview (video only).
    void export_gif(const std::string& path, std::int64_t start_us, std::int64_t end_us,
                    double fps, std::size_t width, std::size_t height,
                    int max_colors = 256, int loop_count = 0) const;

    // Renders the full timeline's audio mix to a WAV file.
    void export_audio(const std::string& path, audio::container::WavEncoding encoding,
                      std::uint32_t sample_rate = 48000, std::uint32_t channels = 2) const;

    // ── Project persistence (JSON) ───────────────────────────────────────────
    void save_project(const std::string& path) const;
    void load_project(const std::string& path);

private:
    MediaBin bin_;
    timeline::Timeline timeline_;
    std::uint64_t next_clip_id_ = 1'000'000; // clear of Timeline::split_clip's own low ids

    mutable std::unordered_map<std::string, audio::AudioBuffer> audio_cache_;
    mutable std::unordered_map<std::string, graphics::Texture> frame_cache_;
};

} // namespace movie_editor
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_MOVIE_EDITOR_CORE_H
