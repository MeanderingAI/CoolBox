#include "gui_gtk_like.hpp"

int main(int argc, char** argv) {
    product::notepad::GtkLikeApp app("CoolBox Notepad");
    (void)argc; (void)argv;
    return app.run();
}
