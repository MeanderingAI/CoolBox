#include "file_browser.hpp"
#include "tyst_framework.hpp"

#include <filesystem>

namespace {

using tools::file_browser::FileBrowserComponent;

TYST_TEST(FileBrowser, RefreshesCurrentWorkspace) {
    FileBrowserComponent browser(std::filesystem::current_path());
    browser.refresh();
    TYST_EXPECT_TRUE(!browser.entries().empty());
}

TYST_TEST(FileBrowser, MissingPathHasNoEntries) {
    FileBrowserComponent browser(std::filesystem::current_path() / "__missing_path__");
    browser.refresh();
    TYST_EXPECT_TRUE(browser.entries().empty());
}

} // namespace