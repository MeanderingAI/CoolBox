#include "abstract_dialog.hpp"

namespace app_builder {
namespace os_generics {

bool is_dialog_request_valid(const DialogRequest& request) {
    if (request.action == DialogAction::SaveFile && request.suggested_name.empty()) {
        return false;
    }
    return true;
}

} // namespace os_generics
} // namespace app_builder
