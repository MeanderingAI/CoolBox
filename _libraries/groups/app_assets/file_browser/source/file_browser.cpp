#include "file_browser.hpp"

#include <algorithm>
#include <system_error>

namespace tools {
namespace file_browser {

namespace fs = std::filesystem;

FileBrowserComponent::FileBrowserComponent() = default;

FileBrowserComponent::FileBrowserComponent(std::filesystem::path root)
    : root_(std::move(root)) {
    refresh();
}

void FileBrowserComponent::set_root(const std::filesystem::path& root) {
    root_ = root;
    refresh();
}

const std::filesystem::path& FileBrowserComponent::root() const {
    return root_;
}

bool FileBrowserComponent::has_root() const {
    return !root_.empty();
}

void FileBrowserComponent::refresh() {
    entries_.clear();
    std::error_code error;
    if (root_.empty() || !fs::exists(root_, error) || !fs::is_directory(root_, error)) {
        return;
    }
    append_entries(root_, 0);
}

const std::vector<FileEntry>& FileBrowserComponent::entries() const {
    return entries_;
}

int FileBrowserComponent::selected_index_for_path(const std::string& path) const {
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].path == path) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

const FileEntry* FileBrowserComponent::entry_at(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= entries_.size()) {
        return nullptr;
    }
    return &entries_[static_cast<std::size_t>(index)];
}

void FileBrowserComponent::append_entries(const std::filesystem::path& path, std::size_t depth) {
    std::error_code error;
    const bool is_directory = fs::is_directory(path, error);
    if (error) {
        return;
    }

    std::string label;
    if (depth == 0) {
        label = path.filename().string();
        if (label.empty()) {
            label = path.string();
        }
    } else {
        label.assign(depth * 2U, ' ');
        label += is_directory ? "> " : "- ";
        label += path.filename().string();
    }

    entries_.push_back({label, path.string(), is_directory, depth});
    if (!is_directory) {
        return;
    }

    std::vector<fs::directory_entry> children;
    for (fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, error);
         !error && it != fs::directory_iterator();
         it.increment(error)) {
        children.push_back(*it);
    }
    if (error) {
        return;
    }

    std::sort(children.begin(), children.end(), [](const fs::directory_entry& left, const fs::directory_entry& right) {
        std::error_code left_error;
        std::error_code right_error;
        const bool left_directory = left.is_directory(left_error);
        const bool right_directory = right.is_directory(right_error);
        if (left_directory != right_directory) {
            return left_directory > right_directory;
        }
        return left.path().filename().string() < right.path().filename().string();
    });

    for (const fs::directory_entry& child : children) {
        append_entries(child.path(), depth + 1U);
    }
}

} // namespace file_browser
} // namespace tools