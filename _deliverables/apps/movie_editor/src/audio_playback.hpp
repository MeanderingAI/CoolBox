#pragma once

#include "movie_editor_core.h"
#include <memory>

namespace movie_editor_app {

class AudioPlayback {
public:
    AudioPlayback();
    ~AudioPlayback();
    AudioPlayback(const AudioPlayback&) = delete;
    AudioPlayback& operator=(const AudioPlayback&) = delete;

    // Explicitly selected only by clock/buffering tests, never a fallback.
    enum class Device { System, NullForTests };
    void start(trekker::movie_editor::EditorProject project, std::int64_t position_us,
               Device device = Device::System);
    void stop();
    std::int64_t position_us() const;
    bool finished() const;
    void check_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
