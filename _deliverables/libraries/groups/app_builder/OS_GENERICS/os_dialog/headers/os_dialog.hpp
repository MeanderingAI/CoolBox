#ifndef COOLBOX_APP_BUILDER_OS_GENERICS_OS_DIALOG_HPP
#define COOLBOX_APP_BUILDER_OS_GENERICS_OS_DIALOG_HPP

#include "abstract_dialog.hpp"

namespace app_builder {
namespace os_generics {

class OsDialog : public AbstractDialog {
public:
    DialogResult show(const DialogRequest& request) override;
};

} // namespace os_generics
} // namespace app_builder

#endif
