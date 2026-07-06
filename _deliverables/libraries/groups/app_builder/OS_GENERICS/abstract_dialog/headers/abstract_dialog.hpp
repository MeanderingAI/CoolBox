#ifndef COOLBOX_APP_BUILDER_OS_GENERICS_ABSTRACT_DIALOG_HPP
#define COOLBOX_APP_BUILDER_OS_GENERICS_ABSTRACT_DIALOG_HPP

#include <string>
#include <vector>

namespace app_builder {
namespace os_generics {

enum class DialogAction {
    OpenFile,
    OpenFiles,
    SaveFile,
    PickFolder
};

struct DialogFilter {
    std::string label;
    std::vector<std::string> patterns;
};

struct DialogRequest {
    DialogAction action = DialogAction::OpenFile;
    std::string title;
    std::string initial_path;
    std::string suggested_name;
    std::vector<DialogFilter> filters;
    bool allow_native_ui = false;
};

struct DialogResult {
    bool accepted = false;
    std::vector<std::string> selected_paths;
    std::string message;
};

class AbstractDialog {
public:
    virtual ~AbstractDialog() = default;
    virtual DialogResult show(const DialogRequest& request) = 0;
};

bool is_dialog_request_valid(const DialogRequest& request);

} // namespace os_generics
} // namespace app_builder

#endif
