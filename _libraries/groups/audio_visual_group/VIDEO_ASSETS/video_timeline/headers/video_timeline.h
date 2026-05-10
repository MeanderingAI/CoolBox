#ifndef TREKKER_VIDEO_ASSETS_VIDEO_TIMELINE_H
#define TREKKER_VIDEO_ASSETS_VIDEO_TIMELINE_H

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace trekker {
namespace timeline {

// ── Transition ────────────────────────────────────────────────────────────────

enum class TransitionType {
    Cut,        // Hard cut (zero duration)
    Dissolve,   // Cross-dissolve / opacity blend
    Wipe,       // Linear wipe (left-to-right)
    Fade,       // Fade to black then in
};

struct Transition {
    TransitionType type        = TransitionType::Cut;
    std::int64_t   duration_us = 0;  // overlap / transition length in μs
};

const char* transition_name(TransitionType t);

// ── Clip ──────────────────────────────────────────────────────────────────────
// Represents one media segment on the timeline.

struct Clip {
    std::uint64_t id          = 0;       // Unique clip ID
    std::string   source_path;           // Media file path
    std::int64_t  in_point_us  = 0;     // Start offset within source (μs)
    std::int64_t  out_point_us = 0;     // End   offset within source (μs); 0 = until EOF
    std::int64_t  position_us  = 0;     // Placement on the timeline (μs)
    float         speed         = 1.f;  // Playback speed multiplier
    float         volume        = 1.f;  // Audio gain for this clip
    bool          muted         = false;
    Transition    transition_in;        // Transition at clip start
    Transition    transition_out;       // Transition at clip end

    // Duration on the timeline (accounting for speed).
    std::int64_t source_duration_us() const {
        return out_point_us - in_point_us;
    }
    std::int64_t timeline_duration_us() const {
        if (speed <= 0.f) throw std::domain_error("speed must be positive");
        return static_cast<std::int64_t>(source_duration_us() / speed);
    }
    std::int64_t timeline_end_us() const {
        return position_us + timeline_duration_us();
    }

    bool contains_pts(std::int64_t pts_us) const {
        return pts_us >= position_us && pts_us < timeline_end_us();
    }
};

// ── Track ─────────────────────────────────────────────────────────────────────

enum class TrackType { Video, Audio };

class Track {
public:
    explicit Track(std::uint64_t id, TrackType type = TrackType::Video,
                   std::string name = {});

    std::uint64_t id()   const { return id_; }
    TrackType     type() const { return type_; }
    const std::string& name() const { return name_; }
    bool muted()  const { return muted_; }
    bool locked() const { return locked_; }
    void set_muted(bool m) { muted_ = m; }
    void set_locked(bool l) { locked_ = l; }

    // Clip management (clips are kept in ascending position order).
    void          add_clip(Clip clip);
    bool          remove_clip(std::uint64_t clip_id);
    const Clip*   clip_at(std::int64_t pts_us) const;
    const std::vector<Clip>& clips() const { return clips_; }

    std::int64_t duration_us() const;

private:
    std::uint64_t   id_;
    TrackType       type_;
    std::string     name_;
    bool            muted_  = false;
    bool            locked_ = false;
    std::vector<Clip> clips_;
};

// ── Timeline ──────────────────────────────────────────────────────────────────

class Timeline {
public:
    Timeline() = default;

    // Track management
    Track&       add_track(TrackType type, const std::string& name = {});
    bool         remove_track(std::uint64_t track_id);
    Track*       track(std::uint64_t track_id);
    const Track* track(std::uint64_t track_id) const;
    const std::vector<Track>& tracks() const { return tracks_; }

    // Timeline-level duration (longest track).
    std::int64_t duration_us() const;

    // Enumerate all clips that overlap the given PTS across all tracks.
    std::vector<const Clip*> clips_at(std::int64_t pts_us) const;

    // Split clip at pts_us; inserts a second clip immediately after.
    // Returns false if no clip covers pts_us on the specified track.
    bool split_clip(std::uint64_t track_id, std::int64_t pts_us);

    // Move all clips in the track at or after start_us by delta_us.
    void shift_clips(std::uint64_t track_id, std::int64_t start_us, std::int64_t delta_us);

    // Simple EDL-style export: one line per clip.
    std::string export_edl() const;

private:
    std::vector<Track>  tracks_;
    std::uint64_t       next_track_id_ = 1;
    std::uint64_t       next_clip_id_  = 1;
};

} // namespace timeline
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_VIDEO_TIMELINE_H
