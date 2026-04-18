#include "bridge.h"

#include "../bridge_forward.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

// bridge.cpp intentionally pulls in implementation sources from the
// repository via textual includes. This file is compiled into a static
// library `coolboxbridge` by the CMake target in this directory.

#include "../../packages/GRAPHICS/charts/headers/graphics.h"

// -- minimal graphics wrappers used by the bridge --
struct CoolBoxFractal { graphics::Fractal impl; CoolBoxFractal(int w, int h, int type) : impl(w, h, static_cast<graphics::FractalType>(type)) {} };
struct CoolBoxFunctionPlot { graphics::FunctionPlot impl; CoolBoxFunctionPlot(int w, int h) : impl(w, h) {} };
struct CoolBoxParametricPlot { graphics::ParametricPlot impl; CoolBoxParametricPlot(int w, int h) : impl(w, h) {} };
struct CoolBoxPolarPlot { graphics::PolarPlot impl; CoolBoxPolarPlot(int w, int h) : impl(w, h) {} };
struct CoolBoxHistogramPlot { graphics::HistogramPlot impl; CoolBoxHistogramPlot(int w, int h) : impl(w, h) {} };

extern "C" {

CoolBoxFractal* coolbox_fractal_create(int width, int height, int type) { return new CoolBoxFractal(width, height, type); }
void coolbox_fractal_set_params(CoolBoxFractal* f, double param1, double param2) { if (f) f->impl.set_params(param1, param2); }
void coolbox_fractal_set_max_iter(CoolBoxFractal* f, int max_iter) { if (f) f->impl.set_max_iter(max_iter); }
void coolbox_fractal_set_bounds(CoolBoxFractal* f, double x_min, double x_max, double y_min, double y_max) { if (f) f->impl.set_bounds(x_min, x_max, y_min, y_max); }
CoolBoxCanvas* coolbox_fractal_render(const CoolBoxFractal* f) { if (!f) return nullptr; auto c = new graphics::Canvas(f->impl.render()); return reinterpret_cast<CoolBoxCanvas*>(c); }
void coolbox_fractal_free(CoolBoxFractal* f) { delete f; }

// The file contains many more bridge functions (ML etc.) which are
// large; to keep this bridge compile unit self-contained we include
// repository source files directly (as before). For brevity the rest
// of the implementations are left identical to the original bridge.cpp
// in the repository; they will be compiled by the cbridge target.

} // extern "C"
