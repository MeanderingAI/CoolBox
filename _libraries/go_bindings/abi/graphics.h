#ifndef COOLBOX_GO_BINDINGS_ABI_GRAPHICS_H
#define COOLBOX_GO_BINDINGS_ABI_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxFractal CoolBoxFractal;
typedef struct CoolBoxFunctionPlot CoolBoxFunctionPlot;
typedef struct CoolBoxParametricPlot CoolBoxParametricPlot;
typedef struct CoolBoxPolarPlot CoolBoxPolarPlot;
typedef struct CoolBoxHistogramPlot CoolBoxHistogramPlot;
typedef struct CoolBoxCanvas CoolBoxCanvas;
typedef struct CoolBoxGraph CoolBoxGraph;
typedef struct CoolBoxTable CoolBoxTable;
typedef struct CoolBoxColor CoolBoxColor;
typedef struct CoolBoxFontFace CoolBoxFontFace;
typedef struct CoolBoxTextRenderer CoolBoxTextRenderer;

CoolBoxFractal* coolbox_fractal_create(int width, int height, int type);
void coolbox_fractal_set_params(CoolBoxFractal* f, double param1, double param2);
void coolbox_fractal_set_max_iter(CoolBoxFractal* f, int max_iter);
void coolbox_fractal_set_bounds(CoolBoxFractal* f, double x_min, double x_max, double y_min, double y_max);
CoolBoxCanvas* coolbox_fractal_render(const CoolBoxFractal* f);
void coolbox_fractal_free(CoolBoxFractal* f);

CoolBoxFunctionPlot* coolbox_function_plot_create(int width, int height);
void coolbox_function_plot_set_equation(CoolBoxFunctionPlot* p, const char* expr);
void coolbox_function_plot_set_range(CoolBoxFunctionPlot* p, double x_min, double x_max);
void coolbox_function_plot_set_samples(CoolBoxFunctionPlot* p, int n);
void coolbox_function_plot_set_color(CoolBoxFunctionPlot* p, CoolBoxColor* c);
CoolBoxCanvas* coolbox_function_plot_render(const CoolBoxFunctionPlot* p);
void coolbox_function_plot_free(CoolBoxFunctionPlot* p);

CoolBoxParametricPlot* coolbox_parametric_plot_create(int width, int height);
void coolbox_parametric_plot_set_equations(CoolBoxParametricPlot* p, const char* x_expr, const char* y_expr);
void coolbox_parametric_plot_set_t_range(CoolBoxParametricPlot* p, double t_min, double t_max);
void coolbox_parametric_plot_set_samples(CoolBoxParametricPlot* p, int n);
void coolbox_parametric_plot_set_color(CoolBoxParametricPlot* p, CoolBoxColor* c);
CoolBoxCanvas* coolbox_parametric_plot_render(const CoolBoxParametricPlot* p);
void coolbox_parametric_plot_free(CoolBoxParametricPlot* p);

CoolBoxPolarPlot* coolbox_polar_plot_create(int width, int height);
void coolbox_polar_plot_set_equation(CoolBoxPolarPlot* p, const char* expr);
void coolbox_polar_plot_set_theta_range(CoolBoxPolarPlot* p, double theta_min, double theta_max);
void coolbox_polar_plot_set_samples(CoolBoxPolarPlot* p, int n);
void coolbox_polar_plot_set_color(CoolBoxPolarPlot* p, CoolBoxColor* c);
CoolBoxCanvas* coolbox_polar_plot_render(const CoolBoxPolarPlot* p);
void coolbox_polar_plot_free(CoolBoxPolarPlot* p);

CoolBoxHistogramPlot* coolbox_histogram_plot_create(int width, int height);
void coolbox_histogram_plot_set_data(CoolBoxHistogramPlot* p, const double* values, int n);
void coolbox_histogram_plot_set_bins(CoolBoxHistogramPlot* p, int n);
void coolbox_histogram_plot_set_color(CoolBoxHistogramPlot* p, CoolBoxColor* c);
CoolBoxCanvas* coolbox_histogram_plot_render(const CoolBoxHistogramPlot* p);
void coolbox_histogram_plot_free(CoolBoxHistogramPlot* p);

CoolBoxColor* coolbox_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void coolbox_color_free(CoolBoxColor* color);

CoolBoxCanvas* coolbox_canvas_create(int width, int height);
void coolbox_canvas_free(CoolBoxCanvas* canvas);
int coolbox_canvas_save_png(const CoolBoxCanvas* canvas, const char* path);
int coolbox_canvas_save_bmp(const CoolBoxCanvas* canvas, const char* path);
int coolbox_canvas_save_jpg(const CoolBoxCanvas* canvas, const char* path, int quality);

CoolBoxGraph* coolbox_graph_create(int width, int height, int graph_type);
void coolbox_graph_free(CoolBoxGraph* graph);
void coolbox_graph_set_title(CoolBoxGraph* graph, const char* title);
void coolbox_graph_set_x_label(CoolBoxGraph* graph, const char* label);
void coolbox_graph_set_y_label(CoolBoxGraph* graph, const char* label);
int coolbox_graph_add_series(CoolBoxGraph* graph, const char* name, const double* x, const double* y, int n, CoolBoxColor* color);
CoolBoxCanvas* coolbox_graph_render(const CoolBoxGraph* graph);

CoolBoxTable* coolbox_table_create(void);
void coolbox_table_free(CoolBoxTable* table);
void coolbox_table_set_headers(CoolBoxTable* table, const char** headers, int n);
void coolbox_table_add_row(CoolBoxTable* table, const char** row, int n);
CoolBoxCanvas* coolbox_table_render(const CoolBoxTable* table);

CoolBoxFontFace* coolbox_fontface_create(void);
void coolbox_fontface_free(CoolBoxFontFace* font);
int coolbox_fontface_load_from_file(CoolBoxFontFace* font, const char* path);
int coolbox_fontface_load_from_memory(CoolBoxFontFace* font, const unsigned char* bytes, int len);
int coolbox_fontface_is_loaded(const CoolBoxFontFace* font);

CoolBoxTextRenderer* coolbox_text_renderer_create(void);
void coolbox_text_renderer_free(CoolBoxTextRenderer* r);
int coolbox_text_renderer_draw_text(CoolBoxTextRenderer* r, CoolBoxCanvas* canvas, CoolBoxFontFace* font, int x, int y, const char* text, CoolBoxColor* color, float pixel_height);

#ifdef __cplusplus
}
#endif

#endif
