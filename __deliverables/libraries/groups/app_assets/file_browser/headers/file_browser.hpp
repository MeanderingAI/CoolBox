#ifndef COOLBOX__LIBRARIES_PACKAGES_TOOLS_FILE_BROWSER_HEADERS_FILE_BROWSER_HPP
#define COOLBOX__LIBRARIES_PACKAGES_TOOLS_FILE_BROWSER_HEADERS_FILE_BROWSER_HPP

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace tools {
namespace file_browser {

struct FileEntry {
    std::string label;
    std::string path;
    bool is_directory = false;
    std::size_t depth = 0;
};

class FileBrowserComponent {
public:
    FileBrowserComponent();
    explicit FileBrowserComponent(std::filesystem::path root);

    void set_root(const std::filesystem::path& root);
    const std::filesystem::path& root() const;
    bool has_root() const;

    void refresh();
    const std::vector<FileEntry>& entries() const;

    int selected_index_for_path(const std::string& path) const;
    const FileEntry* entry_at(int index) const;

private:
    std::filesystem::path root_;
    std::vector<FileEntry> entries_;

    void append_entries(const std::filesystem::path& path, std::size_t depth);
};

} // namespace file_browser
} // namespace tools

#endif  // COOLBOX__LIBRARIES_PACKAGES_TOOLS_FILE_BROWSER_HEADERS_FILE_BROWSER_HPP