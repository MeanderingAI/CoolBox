#include "file_browser.hpp"
#include "tyst_framework.hpp"

#include <chrono>
#include <fstream>
#include <filesystem>

namespace {

using tools::file_browser::FileBrowserComponent;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto unique_id = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("coolbox_file_browser_test_" + std::to_string(unique_id));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

TYST_TEST(FileBrowser, RefreshesTestDirectoryEntries) {
    TemporaryDirectory temp_directory;
    std::filesystem::create_directories(temp_directory.path() / "nested");
    std::ofstream(temp_directory.path() / "sample.txt").put('x');
    std::ofstream(temp_directory.path() / "nested" / "child.txt").put('y');

    FileBrowserComponent browser(temp_directory.path());
    browser.refresh();
    TYST_EXPECT_TRUE(browser.entries().size() >= 3);
}

TYST_TEST(FileBrowser, MissingPathHasNoEntries) {
    TemporaryDirectory temp_directory;
    FileBrowserComponent browser(temp_directory.path() / "__missing_path__");
    browser.refresh();
    TYST_EXPECT_TRUE(browser.entries().empty());
}

} // namespace