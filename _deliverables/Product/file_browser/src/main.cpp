#include "file_browser_app.hpp"

int main(int argc, char** argv) {
    product::file_browser_app::FileBrowserApp app("CoolBox File Browser");
    (void)argc;
    (void)argv;
    return app.run();
}