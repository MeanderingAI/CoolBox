#include "video_timeline.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace trekker {
namespace timeline {

// ── Transition ────────────────────────────────────────────────────────────────

const char* transition_name(TransitionType t) {
    switch (t) {
        case TransitionType::Cut:     return "Cut";
        case TransitionType::Dissolve: return "Dissolve";
        case TransitionType::Wipe:    return "Wipe";
        case TransitionType::Fade:    return "Fade";
    }
    return "Unknown";
}

// ── Track ─────────────────────────────────────────────────────────────────────

Track::Track(std::uint64_t id, TrackType type, std::string name)
    : id_(id), type_(type), name_(std::move(name)) {}

void Track::add_clip(Clip clip) {
    clips_.push_back(std::move(clip));
    std::sort(clips_.begin(), clips_.end(),
              [](const Clip& a, const Clip& b) { return a.position_us < b.position_us; });
}

bool Track::remove_clip(std::uint64_t cid) {
    auto it = std::find_if(clips_.begin(), clips_.end(),
                           [cid](const Clip& c) { return c.id == cid; });
    if (it == clips_.end()) return false;
    clips_.erase(it);
    return true;
}

const Clip* Track::clip_at(std::int64_t pts_us) const {
    for (const auto& c : clips_)
        if (c.contains_pts(pts_us)) return &c;
    return nullptr;
}

std::int64_t Track::duration_us() const {
    std::int64_t d = 0;
    for (const auto& c : clips_) d = std::max(d, c.timeline_end_us());
    return d;
}

// ── Timeline ──────────────────────────────────────────────────────────────────

Track& Timeline::add_track(TrackType type, const std::string& name) {
    tracks_.emplace_back(next_track_id_++, type, name);
    return tracks_.back();
}

bool Timeline::remove_track(std::uint64_t tid) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [tid](const Track& t) { return t.id() == tid; });
    if (it == tracks_.end()) return false;
    tracks_.erase(it);
    return true;
}

Track* Timeline::track(std::uint64_t tid) {
    for (auto& t : tracks_) if (t.id() == tid) return &t;
    return nullptr;
}

const Track* Timeline::track(std::uint64_t tid) const {
    for (const auto& t : tracks_) if (t.id() == tid) return &t;
    return nullptr;
}

std::int64_t Timeline::duration_us() const {
    std::int64_t d = 0;
    for (const auto& t : tracks_) d = std::max(d, t.duration_us());
    return d;
}

std::vector<const Clip*> Timeline::clips_at(std::int64_t pts_us) const {
    std::vector<const Clip*> result;
    for (const auto& t : tracks_)
        if (const auto* c = t.clip_at(pts_us)) result.push_back(c);
    return result;
}

bool Timeline::split_clip(std::uint64_t tid, std::int64_t pts_us) {
    auto* tr = track(tid);
    if (!tr) return false;
    const Clip* existing = tr->clip_at(pts_us);
    if (!existing) return false;

    Clip left  = *existing;
    Clip right = *existing;

    // Compute in-source position for the split point.
    const std::int64_t elapsed_timeline = pts_us - existing->position_us;
    const std::int64_t elapsed_source   = static_cast<std::int64_t>(elapsed_timeline * existing->speed);

    left.id            = next_clip_id_++;
    left.out_point_us  = left.in_point_us + elapsed_source;

    right.id           = next_clip_id_++;
    right.in_point_us  = left.out_point_us;
    right.position_us  = pts_us;

    const std::uint64_t old_id = existing->id;
    tr->remove_clip(old_id);
    tr->add_clip(left);
    tr->add_clip(right);
    return true;
}

void Timeline::shift_clips(std::uint64_t tid, std::int64_t start_us, std::int64_t delta_us) {
    auto* tr = track(tid);
    if (!tr) return;
    // We need to mutate positions — copy-modify-replace.
    std::vector<Clip> updated;
    for (Clip c : tr->clips()) {
        if (c.position_us >= start_us) c.position_us += delta_us;
        updated.push_back(std::move(c));
    }
    // Rebuild: remove all then add back.
    for (auto& c : tr->clips()) ; // iterate to get ids
    std::vector<std::uint64_t> ids;
    for (const auto& c : tr->clips()) ids.push_back(c.id);
    for (auto id : ids) tr->remove_clip(id);
    for (auto& c : updated) tr->add_clip(c);
}

std::string Timeline::export_edl() const {
    std::ostringstream oss;
    oss << "# EDL Export\n";
    for (const auto& t : tracks_) {
        oss << "TRACK " << t.id() << " \"" << t.name() << "\"\n";
        for (const auto& c : t.clips()) {
            oss << "  CLIP " << c.id
                << " src=\"" << c.source_path << "\""
                << " in=" << c.in_point_us
                << " out=" << c.out_point_us
                << " pos=" << c.position_us
                << " speed=" << c.speed
                << " transition_in=" << transition_name(c.transition_in.type)
                << "\n";
        }
    }
    return oss.str();
}

} // namespace timeline
} // namespace trekker
