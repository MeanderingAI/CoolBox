#include "file_editor.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace app_assets {
namespace file_editor {

bool TextFileEditor::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    path_ = path;
    lines_.clear();

    std::string line;
    while (std::getline(in, line)) {
        lines_.push_back(line);
    }

    dirty_ = false;
    return true;
}

bool TextFileEditor::reload() {
    if (path_.empty()) {
        return false;
    }
    return load(path_);
}

bool TextFileEditor::save() {
    if (path_.empty()) {
        return false;
    }

    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    for (std::size_t i = 0; i < lines_.size(); ++i) {
        out << lines_[i];
        if (i + 1 < lines_.size()) {
            out << "\n";
        }
    }

    if (!out) {
        return false;
    }

    dirty_ = false;
    return true;
}

bool TextFileEditor::replace_line(int one_based_line, const std::string& text) {
    if (one_based_line < 1 || one_based_line > static_cast<int>(lines_.size())) {
        return false;
    }
    lines_[static_cast<std::size_t>(one_based_line - 1)] = text;
    dirty_ = true;
    return true;
}

bool TextFileEditor::insert_line(int one_based_line, const std::string& text) {
    if (one_based_line < 1 || one_based_line > static_cast<int>(lines_.size()) + 1) {
        return false;
    }
    lines_.insert(lines_.begin() + (one_based_line - 1), text);
    dirty_ = true;
    return true;
}

bool TextFileEditor::delete_line(int one_based_line) {
    if (one_based_line < 1 || one_based_line > static_cast<int>(lines_.size())) {
        return false;
    }
    lines_.erase(lines_.begin() + (one_based_line - 1));
    dirty_ = true;
    return true;
}

std::vector<std::string> TextFileEditor::view_lines(int one_based_start, int count) const {
    std::vector<std::string> out;
    if (lines_.empty() || count <= 0) {
        return out;
    }

    const int start = std::max(1, one_based_start);
    const int end = std::min(static_cast<int>(lines_.size()), start + count - 1);
    for (int i = start; i <= end; ++i) {
        out.push_back(std::to_string(i) + ": " + lines_[static_cast<std::size_t>(i - 1)]);
    }
    return out;
}

std::string TextFileEditor::text() const {
    std::ostringstream out;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        out << lines_[i];
        if (i + 1 < lines_.size()) {
            out << "\n";
        }
    }
    return out.str();
}

bool open_in_external_editor(const std::string& path) {
    if (path.empty()) {
        return false;
    }

#if defined(_WIN32)
    const std::string command = "cmd /c start \"\" \"" + path + "\"";
    return std::system(command.c_str()) == 0;
#elif defined(__APPLE__)
    const std::string command = "open \"" + path + "\"";
    return std::system(command.c_str()) == 0;
#else
    const std::string command = "xdg-open \"" + path + "\"";
    return std::system(command.c_str()) == 0;
#endif
}

} // namespace file_editor
} // namespace app_assets
