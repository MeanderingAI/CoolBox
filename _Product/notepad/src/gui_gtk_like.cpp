#include "gui_gtk_like.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "windows/headers/windows.hpp"
#include "components/headers/components.hpp"

using namespace graphics;

namespace product {
namespace notepad {

static std::vector<std::string> join_buffer(const std::vector<std::string>& buf) {
    return buf; // components::editable_text_view accepts a single string, but we store lines; assembly done in refresh
}

GtkLikeApp::GtkLikeApp(const std::string &title)
    : title_(title), buffer_({"Welcome to CoolBox Notepad"}), current_path_(), cursor_row_(0), cursor_col_(0) {}

GtkLikeApp::~GtkLikeApp() = default;

void GtkLikeApp::openFile(const std::string &path) {
    std::ifstream in(path);
    buffer_.clear();
    if (!in) { current_path_.clear(); return; }
    std::string line;
    while (std::getline(in, line)) buffer_.push_back(line);
    current_path_ = path;
    cursor_row_ = 0; cursor_col_ = 0;
}

bool GtkLikeApp::saveFile(const std::string &path) {
    std::ofstream out(path);
    if (!out) return false;
    for (size_t i = 0; i < buffer_.size(); ++i) {
        out << buffer_[i];
        if (i + 1 < buffer_.size()) out << '\n';
    }
    current_path_ = path;
    return true;
}

void GtkLikeApp::refresh_window() {
    // Build a WindowSimulator with menu, panels and an editable component
    windows::WindowSimulator win(title_, 80, 24, windows::PlatformStyle::MacOS);

    auto file_menu = windows::Menu("File");
    file_menu.add_item(windows::MenuItem::action("Open", "Ctrl+O"))
             .add_item(windows::MenuItem::action("Save", "Ctrl+S"))
             .add_item(windows::MenuItem::divider())
             .add_item(windows::MenuItem::action("Quit", "Ctrl+Q"));

    // Assemble editable text view
    std::string joined;
    for (size_t i = 0; i < buffer_.size(); ++i) {
        joined += buffer_[i];
        if (i + 1 < buffer_.size()) joined += '\n';
    }

    std::vector<components::Component> widgets = {
        components::Component::editable_text_view(joined, 0, true, 66)
    };

    win.add_menu(file_menu)
       .add_panel(windows::Panel::component_group("Editor", widgets, 16, true))
       .set_status_text(current_path_.empty() ? "Ready" : (std::string("File: ") + current_path_));

    std::cout << win.render() << std::endl;
}

int GtkLikeApp::run() {
    refresh_window();
    std::cout << "Interactive GTK-like editor (keyboard commands). Type 'h' for help." << std::endl;
    for (;;) {
        std::cout << "cmd> ";
        std::string cmd;
        if (!std::getline(std::cin, cmd)) break;
        if (cmd.empty()) { refresh_window(); continue; }
        char c = cmd[0];
        if (c == 'q') break;
        else if (c == 'h') {
            std::cout << "Commands:\n  o - open\n  s - save\n  i - insert line after cursor\n  d - delete current line\n  k/j - move up/down\n  p - print buffer\n  q - quit\n";
        } else if (c == 'o') {
            std::cout << "Open file path: "; std::string p; std::getline(std::cin, p);
            if (!p.empty()) openFile(p);
            refresh_window();
        } else if (c == 's') {
            if (current_path_.empty()) { std::cout << "Save as: "; std::string p; std::getline(std::cin, p); if (!p.empty()) saveFile(p); }
            else saveFile(current_path_);
            refresh_window();
        } else if (c == 'i') {
            std::cout << "Insert line: "; std::string line; std::getline(std::cin, line);
            buffer_.insert(buffer_.begin() + std::min(cursor_row_ + 1, buffer_.size()), line);
            cursor_row_ = std::min(cursor_row_ + 1, buffer_.size()-1);
            cursor_col_ = 0;
            refresh_window();
        } else if (c == 'd') {
            if (!buffer_.empty()) { buffer_.erase(buffer_.begin() + std::min(cursor_row_, buffer_.size()-1)); cursor_row_ = std::min(cursor_row_, buffer_.size() ? buffer_.size()-1 : 0); }
            refresh_window();
        } else if (c == 'k') {
            if (cursor_row_ > 0) --cursor_row_;
            refresh_window();
        } else if (c == 'j') {
            if (cursor_row_ + 1 < buffer_.size()) ++cursor_row_;
            refresh_window();
        } else if (c == 'p') {
            std::cout << "--- FILE START ---\n";
            for (auto &l : buffer_) std::cout << l << "\n";
            std::cout << "--- FILE END ---\n";
        } else {
            std::cout << "Unknown command. 'h' for help.\n";
        }
    }
    return 0;
}

} // namespace notepad
} // namespace product
