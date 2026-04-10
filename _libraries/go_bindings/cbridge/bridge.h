#ifndef COOLBOX__LIBRARIES_GO_BINDINGS_CBRIDGE_BRIDGE_H
#define COOLBOX__LIBRARIES_GO_BINDINGS_CBRIDGE_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Fractal and Plotting Primitives
typedef struct CoolBoxFractal CoolBoxFractal;
typedef struct CoolBoxFunctionPlot CoolBoxFunctionPlot;
typedef struct CoolBoxParametricPlot CoolBoxParametricPlot;
typedef struct CoolBoxPolarPlot CoolBoxPolarPlot;
typedef struct CoolBoxHistogramPlot CoolBoxHistogramPlot;

// Graphics/Chart/Component types for FFI
typedef struct CoolBoxCanvas CoolBoxCanvas;
typedef struct CoolBoxGraph CoolBoxGraph;
typedef struct CoolBoxTable CoolBoxTable;
typedef struct CoolBoxColor CoolBoxColor;
typedef struct CoolBoxFontFace CoolBoxFontFace;
typedef struct CoolBoxTextRenderer CoolBoxTextRenderer;
typedef struct CoolBoxComponent CoolBoxComponent;
typedef struct CoolBoxToolbar CoolBoxToolbar;
typedef struct CoolBoxDockPanel CoolBoxDockPanel;
typedef struct CoolBoxLayerList CoolBoxLayerList;
typedef struct CoolBoxPropertyInspector CoolBoxPropertyInspector;
typedef struct CoolBoxFileTree CoolBoxFileTree;
typedef struct CoolBoxRadioSelector CoolBoxRadioSelector;
typedef struct CoolBoxCheckboxGroup CoolBoxCheckboxGroup;

// Color
CoolBoxColor* coolbox_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void coolbox_color_free(CoolBoxColor* color);

// Canvas
CoolBoxCanvas* coolbox_canvas_create(int width, int height);
void coolbox_canvas_free(CoolBoxCanvas* canvas);
int coolbox_canvas_save_png(const CoolBoxCanvas* canvas, const char* path);
int coolbox_canvas_save_bmp(const CoolBoxCanvas* canvas, const char* path);
int coolbox_canvas_save_jpg(const CoolBoxCanvas* canvas, const char* path, int quality);

// Graph
CoolBoxGraph* coolbox_graph_create(int width, int height, int graph_type);
void coolbox_graph_free(CoolBoxGraph* graph);
void coolbox_graph_set_title(CoolBoxGraph* graph, const char* title);
void coolbox_graph_set_x_label(CoolBoxGraph* graph, const char* label);
void coolbox_graph_set_y_label(CoolBoxGraph* graph, const char* label);
int coolbox_graph_add_series(CoolBoxGraph* graph, const char* name, const double* x, const double* y, int n, CoolBoxColor* color);
CoolBoxCanvas* coolbox_graph_render(const CoolBoxGraph* graph);

// Table
CoolBoxTable* coolbox_table_create(void);
void coolbox_table_free(CoolBoxTable* table);
void coolbox_table_set_headers(CoolBoxTable* table, const char** headers, int n);
void coolbox_table_add_row(CoolBoxTable* table, const char** row, int n);
CoolBoxCanvas* coolbox_table_render(const CoolBoxTable* table);

// FontFace
CoolBoxFontFace* coolbox_fontface_create(void);
void coolbox_fontface_free(CoolBoxFontFace* font);
int coolbox_fontface_load_from_file(CoolBoxFontFace* font, const char* path);
int coolbox_fontface_load_from_memory(CoolBoxFontFace* font, const unsigned char* bytes, int len);
int coolbox_fontface_is_loaded(const CoolBoxFontFace* font);

// TextRenderer
CoolBoxTextRenderer* coolbox_text_renderer_create(void);
void coolbox_text_renderer_free(CoolBoxTextRenderer* r);
int coolbox_text_renderer_draw_text(CoolBoxTextRenderer* r, CoolBoxCanvas* canvas, CoolBoxFontFace* font, int x, int y, const char* text, CoolBoxColor* color, float pixel_height);

// Component primitives (GUI ingredients)
CoolBoxComponent* coolbox_component_create(int component_type);
void coolbox_component_free(CoolBoxComponent* c);

CoolBoxToolbar* coolbox_toolbar_create(const char** actions, int n);
void coolbox_toolbar_free(CoolBoxToolbar* t);

CoolBoxDockPanel* coolbox_dockpanel_create(const char* title, int floating);
void coolbox_dockpanel_free(CoolBoxDockPanel* d);

CoolBoxLayerList* coolbox_layerlist_create(const char** layers, int n, int selected);
void coolbox_layerlist_free(CoolBoxLayerList* l);

CoolBoxPropertyInspector* coolbox_propertyinspector_create(const char** keys, const char** values, int n);
void coolbox_propertyinspector_free(CoolBoxPropertyInspector* p);

CoolBoxFileTree* coolbox_filetree_create(const char* root_name);
void coolbox_filetree_free(CoolBoxFileTree* f);

CoolBoxRadioSelector* coolbox_radioselector_create(const char** options, int n, int selected);
void coolbox_radioselector_free(CoolBoxRadioSelector* r);

CoolBoxCheckboxGroup* coolbox_checkboxgroup_create(const char** options, const int* checked, int n);
void coolbox_checkboxgroup_free(CoolBoxCheckboxGroup* c);

#ifdef __cplusplus
}
#endif
#endif  // COOLBOX__LIBRARIES_GO_BINDINGS_CBRIDGE_BRIDGE_H
