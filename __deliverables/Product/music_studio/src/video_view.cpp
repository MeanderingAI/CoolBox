#include "video_view.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Pull in video_display when available.
#if __has_include(<video_display.hpp>)
#  include <video_display.hpp>
#  define MS_HAS_VIDEO_DISPLAY 1
#endif

namespace music_studio {

struct VideoView::Impl {
    std::string   path;
    bool          playing   = false;
    bool          loop      = false;
    std::int64_t  pos_us    = 0;
    std::int64_t  dur_us    = 0;
    double        rate      = 1.0;
    float         volume    = 1.f;
    bool          tc_overlay = false;

    struct TextOv { std::string text; int x, y; };
    std::vector<TextOv> text_overlays;

    std::size_t fb_w = kDefaultWidth;
    std::size_t fb_h = kDefaultHeight;

    static constexpr std::size_t kDefaultWidth  = VideoView::kDefaultWidth;
    static constexpr std::size_t kDefaultHeight = VideoView::kDefaultHeight;
};

VideoView::VideoView() : impl_(new Impl) {}
VideoView::~VideoView() { delete impl_; }

void VideoView::init() {
    std::cout << "[VideoView] init — " << impl_->fb_w << "x" << impl_->fb_h << "\n";
}

void VideoView::tick(std::int64_t delta_us) {
    if (!impl_->playing) return;
    const std::int64_t advance = static_cast<std::int64_t>(delta_us * impl_->rate);
    impl_->pos_us += advance;
    if (impl_->dur_us > 0 && impl_->pos_us >= impl_->dur_us) {
        if (impl_->loop) impl_->pos_us = 0;
        else { impl_->playing = false; impl_->pos_us = impl_->dur_us; }
    }
}

void VideoView::load(const std::string& path) {
    impl_->path   = path;
    impl_->pos_us = 0;
    impl_->playing = false;
    std::cout << "[VideoView] loaded '" << path << "'\n";
}

void VideoView::play()  { impl_->playing = true;  }
void VideoView::pause() { impl_->playing = false; }
void VideoView::stop()  { impl_->playing = false; impl_->pos_us = 0; }

void VideoView::seek(std::int64_t pts_us) {
    impl_->pos_us = std::max(std::int64_t(0), pts_us);
}

void VideoView::set_playback_rate(double r) {
    impl_->rate = std::max(0.25, std::min(4.0, r));
}

void VideoView::set_volume(float v) {
    impl_->volume = std::max(0.f, std::min(1.f, v));
}

void VideoView::set_loop(bool l) { impl_->loop = l; }

void VideoView::enable_timecode_overlay(bool en) { impl_->tc_overlay = en; }

void VideoView::add_text_overlay(const std::string& text, int x, int y) {
    impl_->text_overlays.push_back({text, x, y});
    std::cout << "[VideoView] overlay added: '" << text << "' at (" << x << "," << y << ")\n";
}

void VideoView::clear_overlays() { impl_->text_overlays.clear(); }

bool         VideoView::is_playing()  const { return impl_->playing;  }
std::int64_t VideoView::position_us() const { return impl_->pos_us;   }
std::int64_t VideoView::duration_us() const { return impl_->dur_us;   }
std::size_t  VideoView::fb_width()    const { return impl_->fb_w;     }
std::size_t  VideoView::fb_height()   const { return impl_->fb_h;     }

} // namespace music_studio
