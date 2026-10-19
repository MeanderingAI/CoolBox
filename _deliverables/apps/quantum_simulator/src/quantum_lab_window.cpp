#include "quantum_lab_window.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace quantum_simulator_app {

namespace {
namespace Colors = graphics::Colors;

std::string bitstring(std::uint64_t value, int num_bits) {
    std::string s(static_cast<std::size_t>(num_bits), '0');
    for (int b = 0; b < num_bits; ++b) {
        if ((value >> b) & 1ULL) s[static_cast<std::size_t>(num_bits - 1 - b)] = '1';
    }
    return s;
}

} // namespace

QuantumLabWindow::QuantumLabWindow(int width, int height)
    : window_(graphics::full_application_window::WindowConfig("CoolBox Quantum Simulator",
                                                              static_cast<std::size_t>(width),
                                                              static_cast<std::size_t>(height),
                                                              true, true)),
      canvas_(width, height, Colors::Black) {
    std::random_device rd;
    rng_.seed((static_cast<std::uint64_t>(rd()) << 32) | rd());
}

QuantumLabWindow::Layout QuantumLabWindow::compute_layout(int width, int height) const {
    Layout l;
    l.width = width;
    l.height = height;
    l.header_y1 = 56;
    l.sidebar_x1 = 380;
    l.chart_y0 = l.header_y1 + 10;
    l.chart_y1 = l.header_y1 + static_cast<int>((height - l.header_y1) * 0.55);
    l.log_y0 = l.chart_y1 + 20;
    return l;
}

std::vector<QuantumLabWindow::ButtonRect> QuantumLabWindow::demo_button_rects(const Layout& layout) const {
    std::vector<ButtonRect> rects;
    const auto& demos = demo_list();
    int y = layout.header_y1 + 16;
    const int row_h = 44;
    for (std::size_t i = 0; i < demos.size(); ++i) {
        rects.push_back({i, 10, y, layout.sidebar_x1 - 10, y + row_h - 6});
        y += row_h;
    }
    return rects;
}

void QuantumLabWindow::run_demo_by_index(std::size_t index) {
    const auto& demos = demo_list();
    if (index >= demos.size()) return;
    last_result_ = run_demo(demos[index].id, rng_);
    last_demo_index_ = index;
    log_scroll_ = 0;
}

void QuantumLabWindow::handle_click(int x, int y) {
    const Layout l = compute_layout(canvas_.width(), canvas_.height());
    for (const auto& button : demo_button_rects(l)) {
        if (x >= button.x0 && x < button.x1 && y >= button.y0 && y < button.y1) {
            run_demo_by_index(button.index);
            return;
        }
    }
}

void QuantumLabWindow::render_scene() {
    int cw = canvas_.width(), ch = canvas_.height();
    window_.client_size(cw, ch);
    if (cw > 0 && ch > 0 && (cw != canvas_.width() || ch != canvas_.height())) {
        canvas_ = graphics::Canvas(cw, ch, Colors::Black);
    }
    const Layout l = compute_layout(canvas_.width(), canvas_.height());
    graphics::Canvas& canvas = canvas_;
    canvas.fill(Colors::Black);

    // Header.
    canvas.draw_rect(0, 0, l.width, l.header_y1, Colors::DarkGray, true);
    canvas.draw_text(10, 10, "COOLBOX QUANTUM SIMULATOR", Colors::Cyan, 2);
    if (last_result_) {
        canvas.draw_text(10, 34, last_result_->title, Colors::White, 1);
    } else {
        canvas.draw_text(10, 34, "Select a demo on the left to run it.", Colors::LightGray, 1);
    }

    // Sidebar: demo buttons.
    canvas.draw_rect(0, l.header_y1, l.sidebar_x1, l.height - l.header_y1, Colors::Black, true);
    canvas.draw_line(l.sidebar_x1, l.header_y1, l.sidebar_x1, l.height, Colors::DarkGray, 1);
    const auto& demos = demo_list();
    for (const auto& button : demo_button_rects(l)) {
        const bool is_selected = last_demo_index_ && *last_demo_index_ == button.index;
        canvas.draw_rect(button.x0, button.y0, button.x1 - button.x0, button.y1 - button.y0,
                         is_selected ? Colors::Blue : Colors::DarkGray, true);
        canvas.draw_rect(button.x0, button.y0, button.x1 - button.x0, button.y1 - button.y0, Colors::Gray, false);
        canvas.draw_text(button.x0 + 8, button.y0 + 10, demos[button.index].label, Colors::White, 1);
    }

    // Bar chart (main area, top half).
    const int chart_x0 = l.sidebar_x1 + 20;
    const int chart_x1 = l.width - 20;
    canvas.draw_text(chart_x0, l.header_y1 + 6, "OUTCOME PROBABILITIES", Colors::Cyan, 1);
    canvas.draw_line(chart_x0, l.chart_y1, chart_x1, l.chart_y1, Colors::DarkGray, 1);

    if (last_result_ && !last_result_->probabilities.empty()) {
        const auto& probs = last_result_->probabilities;
        const int n_bits = last_result_->num_qubits_for_display;
        const std::size_t num_bars = probs.size();
        const int available_w = chart_x1 - chart_x0;
        const int bar_slot = std::max(4, available_w / static_cast<int>(num_bars));
        const int bar_w = std::max(2, bar_slot - 4);
        const int chart_top = l.chart_y0 + 20;
        const int chart_bottom = l.chart_y1 - 24;
        const int chart_h = std::max(1, chart_bottom - chart_top);

        for (std::size_t i = 0; i < num_bars; ++i) {
            const int x = chart_x0 + static_cast<int>(i) * bar_slot;
            const int bar_h = static_cast<int>(std::round(probs[i] * chart_h));
            const int y0 = chart_bottom - bar_h;
            if (bar_h > 0) {
                canvas.draw_rect(x, y0, bar_w, bar_h, Colors::Green, true);
            }
            if (num_bars <= 16) {
                canvas.draw_text(x, chart_bottom + 4, bitstring(i, n_bits), Colors::LightGray, 1);
            }
        }
    } else {
        canvas.draw_text(chart_x0, l.chart_y0 + 20, "(this demo has no single final state to chart -- see the log below)",
                         Colors::DarkGray, 1);
    }

    // Narration log (main area, bottom half).
    canvas.draw_text(chart_x0, l.log_y0, "LOG", Colors::Cyan, 1);
    int log_y = l.log_y0 + 18;
    const int log_row_h = 16;
    if (last_result_) {
        if (!last_result_->headline.empty()) {
            canvas.draw_text(chart_x0, log_y, ">> " + last_result_->headline, Colors::Orange, 1);
            log_y += log_row_h + 4;
        }
        for (const auto& line : last_result_->log_lines) {
            if (log_y + log_row_h > l.height - 10) break;
            canvas.draw_text(chart_x0, log_y, line, Colors::LightGray, 1);
            log_y += log_row_h;
        }
    }

    window_.present_canvas(canvas);
}

bool QuantumLabWindow::run() {
    graphics::full_application_window::RenderHooks hooks;
    hooks.on_render = [this](const graphics::full_application_window::RenderEvent&) { render_scene(); };
    window_.set_render_hooks(std::move(hooks));

    if (!window_.create()) return false;
    window_.show();

    bool prev_left_down = false;
    while (window_.pump_events()) {
        graphics::full_application_window::PointerState pointer{};
        if (window_.query_pointer_state(pointer)) {
            const bool left_pressed = pointer.left_button_down && !prev_left_down;
            if (left_pressed && pointer.inside) {
                handle_click(pointer.x, pointer.y);
            }
            prev_left_down = pointer.left_button_down;
        }
        render_scene();
    }
    return true;
}

} // namespace quantum_simulator_app
