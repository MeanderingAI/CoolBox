#ifndef COOLBOX_GO_GRAPHICS_BRIDGE_H
#define COOLBOX_GO_GRAPHICS_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* coolbox_graph_new(int width, int height, int type,
                        char* error, size_t error_capacity);
void coolbox_graph_free(void* graph);
int coolbox_graph_set_title(void* graph, const char* title,
                            char* error, size_t error_capacity);
int coolbox_graph_set_x_label(void* graph, const char* label,
                              char* error, size_t error_capacity);
int coolbox_graph_set_y_label(void* graph, const char* label,
                              char* error, size_t error_capacity);
int coolbox_graph_add_series(void* graph, const char* label,
                             const double* x_values, const double* y_values,
                             size_t count, uint8_t red, uint8_t green,
                             uint8_t blue, uint8_t alpha,
                             char* error, size_t error_capacity);
void* coolbox_graph_render(void* graph, char* error, size_t error_capacity);
void* coolbox_plot_2d(const double* x_values, const double* y_values, size_t count,
                      const char* title, const char* x_label, const char* y_label,
                      const char* series_label, uint8_t red, uint8_t green,
                      uint8_t blue, uint8_t alpha, int width, int height, int type,
                      char* error, size_t error_capacity);
void* coolbox_plot_3d(const double* x_values, const double* y_values,
                      const double* z_values, size_t count, const char* title,
                      const char* x_label, const char* y_label, const char* z_label,
                      const char* series_label, uint8_t red, uint8_t green,
                      uint8_t blue, uint8_t alpha, int width, int height, int type,
                      double azimuth_degrees, double elevation_degrees,
                      char* error, size_t error_capacity);
int coolbox_canvas_show_window(void* canvas, const char* title, const char* save_path,
                               char* error, size_t error_capacity);

void coolbox_canvas_free(void* canvas);
int coolbox_canvas_width(void* canvas);
int coolbox_canvas_height(void* canvas);
size_t coolbox_canvas_data_size(void* canvas);
int coolbox_canvas_copy_data(void* canvas, uint8_t* output, size_t capacity,
                             char* error, size_t error_capacity);
int coolbox_canvas_save(void* canvas, const char* path, int format, int quality,
                        char* error, size_t error_capacity);

#ifdef __cplusplus
}
#endif

#endif
