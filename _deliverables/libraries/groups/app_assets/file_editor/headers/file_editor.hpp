#ifndef COOLBOX_LIBRARIES_GROUPS_APP_ASSETS_FILE_EDITOR_HEADERS_FILE_EDITOR_HPP
#define COOLBOX_LIBRARIES_GROUPS_APP_ASSETS_FILE_EDITOR_HEADERS_FILE_EDITOR_HPP

#include <string>
#include <vector>

namespace app_assets {
namespace file_editor {

class TextFileEditor {
public:
    bool load(const std::string& path);
    bool reload();
    bool save();

    bool replace_line(int one_based_line, const std::string& text);
    bool insert_line(int one_based_line, const std::string& text);
    bool delete_line(int one_based_line);

    std::vector<std::string> view_lines(int one_based_start, int count) const;

    const std::string& path() const { return path_; }
    const std::vector<std::string>& lines() const { return lines_; }
    std::string text() const;
    bool dirty() const { return dirty_; }

private:
    std::string path_;
    std::vector<std::string> lines_;
    bool dirty_ = false;
};

bool open_in_external_editor(const std::string& path);

} // namespace file_editor
} // namespace app_assets

#endif
