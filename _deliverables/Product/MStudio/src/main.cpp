#include "gui_gtk_like.hpp"

int main(int argc, char** argv) {
    product::mstudio::GtkLikeApp app("CoolBox MStudio");
    (void)argc;
    (void)argv;
    return app.run();
}
