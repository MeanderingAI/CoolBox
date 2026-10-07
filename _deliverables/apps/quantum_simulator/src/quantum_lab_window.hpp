#ifndef COOLBOX_APPS_QUANTUM_SIMULATOR_QUANTUM_LAB_WINDOW_HPP
#define COOLBOX_APPS_QUANTUM_SIMULATOR_QUANTUM_LAB_WINDOW_HPP

#include "demo_runner.hpp"

#include "full_application_window.hpp"
#include "graphics.h"

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace quantum_simulator_app {

// A canvas-drawn GUI for browsing/running the demo algorithms: a list of
// presets on the left (click to run), a probability bar chart in the main
// area, and a scrolling narration log beneath it.
//
// Known limitation (by design, not an oversight): the underlying
// full_application_window library has no keyboard/text-input support yet
// (only mouse-position polling), so parameters like "which N to factor"
// are chosen from a fixed preset list via buttons rather than typed in
// freely. Real text entry (e.g. to factor an arbitrary N) would require
// adding keyboard event plumbing to full_application_window first.
class QuantumLabWindow {
public:
    QuantumLabWindow(int width = 1400, int height = 900);

    // Runs the window's event loop until closed. Returns true on a clean
    // exit, false if window creation failed.
    bool run();

private:
    struct Layout {
        int width = 0;
        int height = 0;
        int sidebar_x1 = 0;
        int header_y1 = 0;
        int chart_y0 = 0;
        int chart_y1 = 0;
        int log_y0 = 0;
    };

    struct ButtonRect {
        std::size_t index;
        int x0, y0, x1, y1;
    };

    graphics::full_application_window::FullApplicationWindow window_;
    graphics::Canvas canvas_;

    std::mt19937_64 rng_;
    std::optional<DemoResult> last_result_;
    std::optional<std::size_t> last_demo_index_;
    std::size_t log_scroll_ = 0; // index of the first visible log line

    Layout compute_layout(int width, int height) const;
    std::vector<ButtonRect> demo_button_rects(const Layout& layout) const;

    void render_scene();
    void handle_click(int x, int y);
    void run_demo_by_index(std::size_t index);
};

} // namespace quantum_simulator_app

#endif // COOLBOX_APPS_QUANTUM_SIMULATOR_QUANTUM_LAB_WINDOW_HPP
