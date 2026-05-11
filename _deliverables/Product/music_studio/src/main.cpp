// music_studio — main entry point
// Initialises the GUI framework window and mounts the four views:
//   SequencerView, MixerView, TheoryView, VideoView.
//
// The actual widget toolkit is provided by full_application_window (when
// available at link time).  When the library is absent the application still
// compiles and runs as a headless/console stub — useful for CI builds.

#include "sequencer_view.hpp"
#include "mixer_view.hpp"
#include "theory_view.hpp"
#include "video_view.hpp"

#include <iostream>
#include <string>

// ── Minimal fallback declarations so we compile without the GUI library ───────
#ifndef COOLBOX_FULL_APPLICATION_WINDOW_H
struct AppWindow {
    int width = 1280, height = 800;
    std::string title;
    bool running = false;
    void show() { running = true; }
    void close() { running = false; }
};
#endif

// ── Application ───────────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    AppWindow win;
    win.title  = "Music Studio";
    win.width  = 1440;
    win.height = 900;

    music_studio::SequencerView sequencer;
    music_studio::MixerView     mixer;
    music_studio::TheoryView    theory;
    music_studio::VideoView     video;

    sequencer.init();
    mixer.init();
    theory.init();
    video.init();

    std::cout << "[music_studio] starting — " << win.width << "x" << win.height << "\n";

    win.show();

    // Main loop stub (real implementation delegates to the GUI framework).
    while (win.running) {
        sequencer.tick(16'666); // ~60 fps tick
        mixer.tick(16'666);
        theory.tick(16'666);
        video.tick(16'666);
        win.close(); // headless: exit after one frame
    }

    std::cout << "[music_studio] exiting cleanly\n";
    return 0;
}
