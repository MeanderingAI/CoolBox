#pragma once
#include <string>
#include <vector>
#include <cstddef>

namespace product {
namespace notepad {

class GtkLikeApp {
public:
    explicit GtkLikeApp(const std::string &title = "CoolBox Notepad");
    ~GtkLikeApp();

    int run();
    void openFile(const std::string &path);
    bool saveFile(const std::string &path);

private:
    std::string title_;
    std::vector<std::string> buffer_;
    std::string current_path_;
    std::size_t cursor_row_ = 0;
    std::size_t cursor_col_ = 0;
    void refresh_window();
};

} // namespace notepad
} // namespace product
