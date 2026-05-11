#include <full_application_window.hpp>
#include "body_generator.hpp"
#include <iostream>

using namespace graphics::full_application_window;

void render_body(const RenderEvent& event) {
    // Placeholder: just clear and print frame info
    std::cout << "Rendering frame " << event.frame_index << " (" << event.width << "x" << event.height << ")\n";
    // TODO: draw the mesh here using your graphics library
}

int main() {
    WindowConfig config("Body Generator", 1280, 800, true, true);
    FullApplicationWindow window(config);
    RenderHooks hooks;
    hooks.on_render = render_body;
    window.set_render_hooks(hooks);
    if (!window.create()) {
        std::cerr << "Failed to create window!\n";
        return 1;
    }
    window.show();
    while (window.is_open()) {
        window.pump_events();
    }
    return 0;
}
