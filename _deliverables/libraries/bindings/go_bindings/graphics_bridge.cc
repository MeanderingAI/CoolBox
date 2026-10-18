#include "graphics_bridge.h"

#include <graphics.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GraphHandle {
    explicit GraphHandle(int width, int height, graphics::GraphType type)
        : value(width, height, type) {}
    graphics::Graph value;
};

void write_error(char* output, size_t capacity, const char* message) {
    if (!output || capacity == 0) return;
    const size_t count = std::min(capacity - 1, std::strlen(message));
    std::memcpy(output, message, count);
    output[count] = '\0';
}

template <typename Function>
int invoke(char* error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        write_error(error, capacity, exception.what());
    } catch (...) {
        write_error(error, capacity, "unknown native exception");
    }
    return 0;
}

GraphHandle& graph(void* handle) {
    if (!handle) throw std::invalid_argument("graph is closed");
    return *static_cast<GraphHandle*>(handle);
}

graphics::Canvas& canvas(void* handle) {
    if (!handle) throw std::invalid_argument("canvas is closed");
    return *static_cast<graphics::Canvas*>(handle);
}

} // namespace

extern "C" {

void* coolbox_graph_new(int width, int height, int type,
                        char* error, size_t error_capacity) {
    try {
        if (type < 0 || type > 2) throw std::invalid_argument("unknown graph type");
        return new GraphHandle(width, height, static_cast<graphics::GraphType>(type));
    } catch (const std::exception& exception) {
        write_error(error, error_capacity, exception.what());
    } catch (...) {
        write_error(error, error_capacity, "unknown native exception");
    }
    return nullptr;
}

void coolbox_graph_free(void* handle) { delete static_cast<GraphHandle*>(handle); }

int coolbox_graph_set_title(void* handle, const char* title, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!title) throw std::invalid_argument("title is null");
        graph(handle).value.set_title(title);
    });
}

int coolbox_graph_set_x_label(void* handle, const char* label, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!label) throw std::invalid_argument("x label is null");
        graph(handle).value.set_x_label(label);
    });
}

int coolbox_graph_set_y_label(void* handle, const char* label, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!label) throw std::invalid_argument("y label is null");
        graph(handle).value.set_y_label(label);
    });
}

int coolbox_graph_add_series(void* handle, const char* label,
                             const double* x_values, const double* y_values,
                             size_t count, uint8_t red, uint8_t green,
                             uint8_t blue, uint8_t alpha,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!label || count == 0 || !x_values || !y_values) {
            throw std::invalid_argument("series label and non-empty x/y data are required");
        }
        graphics::DataSeries series;
        series.label = label;
        series.x_values.assign(x_values, x_values + count);
        series.y_values.assign(y_values, y_values + count);
        series.color = {red, green, blue, alpha};
        graph(handle).value.add_series(series);
    });
}

void* coolbox_graph_render(void* handle, char* error, size_t error_capacity) {
    try {
        return new graphics::Canvas(graph(handle).value.render());
    } catch (const std::exception& exception) {
        write_error(error, error_capacity, exception.what());
    } catch (...) {
        write_error(error, error_capacity, "unknown native exception");
    }
    return nullptr;
}

void coolbox_canvas_free(void* handle) { delete static_cast<graphics::Canvas*>(handle); }
int coolbox_canvas_width(void* handle) { return handle ? canvas(handle).width() : 0; }
int coolbox_canvas_height(void* handle) { return handle ? canvas(handle).height() : 0; }
size_t coolbox_canvas_data_size(void* handle) {
    if (!handle) return 0;
    return static_cast<size_t>(canvas(handle).width()) * canvas(handle).height() * 4;
}

int coolbox_canvas_copy_data(void* handle, uint8_t* output, size_t capacity,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        const size_t required = coolbox_canvas_data_size(handle);
        if (!output || capacity < required) throw std::invalid_argument("canvas pixel output buffer is too small");
        std::copy_n(canvas(handle).data(), required, output);
    });
}

int coolbox_canvas_save(void* handle, const char* path, int format, int quality,
                        char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!path || path[0] == '\0') throw std::invalid_argument("output path is empty");
        bool saved = false;
        switch (format) {
        case 0: saved = canvas(handle).save_bmp(path); break;
        case 1: saved = canvas(handle).save_png(path); break;
        case 2:
            if (quality < 1 || quality > 100) throw std::invalid_argument("JPEG quality must be between 1 and 100");
            saved = canvas(handle).save_jpg(path, quality);
            break;
        default: throw std::invalid_argument("unknown image format");
        }
        if (!saved) throw std::runtime_error("failed to write image file");
    });
}

} // extern "C"
