#include "abstract_dialog.hpp"

#include "tyst_framework.hpp"

namespace {

using app_builder::os_generics::DialogAction;
using app_builder::os_generics::DialogRequest;
using app_builder::os_generics::is_dialog_request_valid;

TYST_TEST(AbstractDialogTest, SaveFileRequiresSuggestedName) {
    DialogRequest request;
    request.action = DialogAction::SaveFile;

    TYST_ASSERT_EQ(is_dialog_request_valid(request), false);

    request.suggested_name = "output.hk_cad";
    TYST_ASSERT_EQ(is_dialog_request_valid(request), true);
}

TYST_TEST(AbstractDialogTest, OpenFileWithoutSuggestedNameIsValid) {
    DialogRequest request;
    request.action = DialogAction::OpenFile;
    TYST_ASSERT_EQ(is_dialog_request_valid(request), true);
}

} // namespace
