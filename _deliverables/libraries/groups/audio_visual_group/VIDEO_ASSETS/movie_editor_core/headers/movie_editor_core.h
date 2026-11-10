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
#include <memory>

namespace trekker {
namespace movie_editor {

using TextStyle = timeline::TextStyle;
struct TextLayout { int x, y, width, height; };
// Pixel bounds use the same font and normalized anchors as final rendering.
TextLayout text_layout(const TextStyle& style, std::size_t width, std::size_t height);

// Throws invalid_argument for empty/NUL-containing/>16384-byte text, non-RGB colors,
// font_size outside [1,720], nonfinite/out-of-range normalized anchors,
// or negative fades. Font size scales with output height / 720.
void validate_text_style(const TextStyle& style);

// ── Media bin ─────────────────────────────────────────────────────────────────

enum class MediaType { ImageSequence, StillImage, Audio, ImportedVideo };

struct VideoImportCache {
    std::string directory;
    ~VideoImportCache();
};

struct MediaAsset {
    MediaType type = MediaType::StillImage;
    std::string path;                      // directory (ImageSequence) or file path
    double fps = 24.0;                     // image sequence or imported video
    std::vector<std::string> frame_files;  // sorted absolute frame paths (size 1 for stills)
    std::int64_t duration_us = 0;
    std::uint32_t audio_sample_rate = 0;   // Audio only
    std::uint32_t audio_channels = 0;      // Audio only

    // Native GIF/AVI imports retain decoded frames; FFmpeg imports instead
    // use frame_files backed by the owned temporary import_cache.
    std::vector<video::VideoFrame> decoded_frames;
    std::vector<std::int64_t> frame_start_us; // same length as decoded_frames; cumulative
    std::shared_ptr<VideoImportCache> import_cache;
    std::string audio_path; // FFmpeg-extracted PCM WAV, owned by import_cache
};

// Creates lossless PNG editing frames at 24 fps and optional PCM audio using FFmpeg.
MediaAsset decode_ffmpeg_video(const std::string& path);

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

    // GIF and uncompressed AVI use the built-in readers. Other formats
    // (including compressed AVI) use FFmpeg disk-backed editing frames and
    // PCM audio. Throws if decoding fails or FFmpeg is unavailable.
    std::string import_video_file(const std::string& path);
    // Transfers ownership of an asynchronously decoded media asset.
    std::string add_asset(MediaAsset asset);

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
    // Locked/missing tracks are unchanged; successful deletion is undoable.
    bool delete_track(std::uint64_t track_id);
    // Creating a matching track and adding its first clip is one history entry.
    std::uint64_t add_clip_on_new_track(const std::string& media_key, std::int64_t position_us);

    // Creates a highest video track and its title/overlay as one undo entry.
    // Add/edit/load fail before mutation if a system TrueType font is unavailable.
    std::uint64_t add_text_clip(const TextStyle& style, std::int64_t position_us,
                                std::int64_t duration_us);
    // Retains placement/ID, resets source range and speed to the new duration.
    void edit_text_clip(std::uint64_t track, std::uint64_t clip,
                        const TextStyle& style, std::int64_t duration_us);
    const TextStyle* text_style(std::uint64_t track, std::uint64_t clip) const;

    // Adds a clip referencing an already-imported media asset (keyed by its
    // MediaBin path/directory) to `track_id` at `position_us`. in_us/out_us
    // trim the source; out_us == 0 means "use the asset's full duration".
    // Throws std::invalid_argument if the track or media key is unknown.
    std::uint64_t add_clip(std::uint64_t track_id, const std::string& media_key,
                           std::int64_t position_us,
                           std::int64_t in_us = 0, std::int64_t out_us = 0);

    bool split_clip(std::uint64_t track_id, std::int64_t pts_us);
    bool delete_clip(std::uint64_t track_id, std::uint64_t clip_id);
    void move_clip(std::uint64_t from_track, std::uint64_t clip_id,
                   std::uint64_t to_track, std::int64_t position_us);
    void trim_clip(std::uint64_t track_id, std::uint64_t clip_id,
                   std::int64_t in_us, std::int64_t out_us, std::int64_t position_us);
    bool undo();
    bool redo();
    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }

    std::int64_t duration_us() const { return timeline_.duration_us(); }

    // Renders the composited frame visible at `pts_us` at (out_width x
    // out_height), bottom-to-top. Ordinary video is opaque; text and optional
    // full-frame backgrounds share alpha=min(fade-in,fade-out), measured
    // from each clip's current timeline boundaries (also after trim/split).
    // Anchors place text within the available frame; multiline is supported.
    // Throws if a system TrueType font cannot be loaded. Empty output is black.
    video::VideoFrame render_frame_at(std::int64_t pts_us, std::size_t out_width, std::size_t out_height) const;
    // Drafts omit fades and replace the identified clip, or add a top layer.
    video::VideoFrame render_text_preview(const TextStyle& style, std::int64_t pts_us,
        std::size_t width, std::size_t height, std::uint64_t exclude_track = 0,
        std::uint64_t exclude_clip = 0) const;

    // Mixes every unmuted audio clip overlapping [start_us, end_us) down to
    // one AudioBuffer at (sample_rate, channels), applying per-clip volume
    // and resampling as needed. Speed changes resample audio along with
    // video (and change pitch); pitch-preserving time stretching is not used.
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
    void export_mp4(const std::string& path, double fps = 24.0,
                    std::size_t width = 1280, std::size_t height = 720) const;

    // ── Project persistence (JSON) ───────────────────────────────────────────
    void save_project(const std::string& path) const;
    void load_project(const std::string& path);

private:
    video::VideoFrame render_frame_excluding(std::int64_t pts_us, std::size_t width, std::size_t height,
        std::uint64_t exclude_track, std::uint64_t exclude_clip, const TextStyle* replacement = nullptr) const;
    void checkpoint();
    std::vector<timeline::Timeline> undo_;
    std::vector<timeline::Timeline> redo_;
    MediaBin bin_;
    timeline::Timeline timeline_;
    std::uint64_t next_clip_id_ = 1'000'000; // clear of Timeline::split_clip's own low ids

    mutable std::unordered_map<std::string, audio::AudioBuffer> audio_cache_;
    mutable std::unordered_map<std::string, graphics::Texture> frame_cache_;
};

} // namespace movie_editor
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_MOVIE_EDITOR_CORE_H
