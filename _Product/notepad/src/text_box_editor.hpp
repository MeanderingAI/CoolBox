#pragma once
#include <string>
#include <vector>

namespace product {
namespace notepad {

class TextBoxEditor {
public:
    explicit TextBoxEditor(const std::string &title = "TextBoxEditor");
    ~TextBoxEditor();

    // Run main loop
    int run();

    void openFile(const std::string &path);
    bool saveFile(const std::string &path);

private:
    std::string title_;
    std::vector<std::string> buffer_;
    std::string current_path_;
};

} // namespace notepad
} // namespace product
