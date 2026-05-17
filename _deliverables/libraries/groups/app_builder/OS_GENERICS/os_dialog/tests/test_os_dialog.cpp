#include "os_dialog.hpp"

#include "tyst_framework.hpp"

#include <filesystem>
#include <fstream>

namespace {

using app_builder::os_generics::DialogAction;
using app_builder::os_generics::DialogRequest;
using app_builder::os_generics::OsDialog;

TYST_TEST(OsDialogTest, OpenFileAcceptsExistingInitialPath) {
    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "os_dialog_existing.txt";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << "ok";
    }

    OsDialog dialog;
    DialogRequest request;
    request.action = DialogAction::OpenFile;
    request.initial_path = temp.string();

    const auto result = dialog.show(request);
    TYST_ASSERT_EQ(result.accepted, true);
    TYST_ASSERT_EQ(result.selected_paths.empty(), false);

    std::filesystem::remove(temp);
}

TYST_TEST(OsDialogTest, SaveFileReturnsResolvedTarget) {
    OsDialog dialog;
    DialogRequest request;
    request.action = DialogAction::SaveFile;
    request.initial_path = std::filesystem::temp_directory_path().string();
    request.suggested_name = "new_output.step";

    const auto result = dialog.show(request);
    TYST_ASSERT_EQ(result.accepted, true);
    TYST_ASSERT_EQ(result.selected_paths.empty(), false);
}

TYST_TEST(OsDialogTest, PickFolderRejectsMissingPath) {
    OsDialog dialog;
    DialogRequest request;
    request.action = DialogAction::PickFolder;
    request.initial_path = "C:/definitely_missing_folder_for_test_2983";

    const auto result = dialog.show(request);
    TYST_ASSERT_EQ(result.accepted, false);
}

} // namespace
