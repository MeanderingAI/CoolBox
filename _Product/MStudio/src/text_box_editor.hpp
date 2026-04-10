#ifndef COOLBOX__PRODUCT_MSTUDIO_SRC_TEXT_BOX_EDITOR_HPP
#define COOLBOX__PRODUCT_MSTUDIO_SRC_TEXT_BOX_EDITOR_HPP
#include <string>
#include <vector>

namespace product {
namespace mstudio {

class TextBoxEditor {
public:
    explicit TextBoxEditor(const std::string &title = "TextBoxEditor");
    ~TextBoxEditor();

    int run();

    void openFile(const std::string &path);
    bool saveFile(const std::string &path);

private:
    std::string title_;
    std::vector<std::string> buffer_;
    std::string current_path_;
};

} // namespace mstudio
} // namespace product
#endif  // COOLBOX__PRODUCT_MSTUDIO_SRC_TEXT_BOX_EDITOR_HPP
