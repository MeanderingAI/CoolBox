#include "text_box_editor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace product {
namespace notepad {

TextBoxEditor::TextBoxEditor(const std::string &title)
    : title_(title), buffer_(), current_path_()
{
}

TextBoxEditor::~TextBoxEditor() = default;

void TextBoxEditor::openFile(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    buffer_.clear();
    if (!in) {
        current_path_.clear();
        return;
    }
    std::string line;
    while (std::getline(in, line)) buffer_.push_back(line);
    current_path_ = path;
}

bool TextBoxEditor::saveFile(const std::string &path) {
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out) return false;
    for (size_t i = 0; i < buffer_.size(); ++i) {
        out << buffer_[i];
        if (i + 1 < buffer_.size()) out << '\n';
    }
    current_path_ = path;
    return true;
}

int TextBoxEditor::run() {
    std::cout << "=== " << title_ << " ===\n";
    std::cout << "Commands: (o)pen (s)ave (e)dit (p)rint (q)uit\n";
    for (;;) {
        std::cout << "cmd> ";
        std::string cmd;
        if (!std::getline(std::cin, cmd)) break;
        if (cmd.empty()) continue;
        char c = cmd[0];
        if (c == 'q') break;
        else if (c == 'o') {
            std::cout << "Open file path: ";
            std::string path; std::getline(std::cin, path);
            openFile(path);
            std::cout << "Loaded " << buffer_.size() << " lines.\n";
        } else if (c == 's') {
            if (current_path_.empty()) {
                std::cout << "Save as path: ";
                std::string path; std::getline(std::cin, path);
                if (saveFile(path)) std::cout << "Saved.\n";
                else std::cout << "Save failed.\n";
            } else {
                if (saveFile(current_path_)) std::cout << "Saved.\n";
                else std::cout << "Save failed.\n";
            }
        } else if (c == 'e') {
            std::cout << "Enter text (single line). Empty line to stop.\n";
            for (;;) {
                std::string line; std::getline(std::cin, line);
                if (line.empty()) break;
                buffer_.push_back(line);
            }
            std::cout << "Appended. Buffer lines: " << buffer_.size() << "\n";
        } else if (c == 'p') {
            std::cout << "--- FILE START ---\n";
            for (auto &l : buffer_) std::cout << l << "\n";
            std::cout << "--- FILE END ---\n";
        } else {
            std::cout << "Unknown command. Use o/s/e/p/q.\n";
        }
    }
    return 0;
}

} // namespace notepad
} // namespace product
