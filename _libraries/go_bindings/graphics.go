package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/graphics.h"
*/
import "C"

import (
	"fmt"
	"runtime"
	"unsafe"
)

type Color struct{ handle *C.CoolBoxColor }

func NewColor(r, g, b, a uint8) *Color {
	h := C.coolbox_color_create(C.uchar(r), C.uchar(g), C.uchar(b), C.uchar(a))
	c := &Color{handle: h}
	runtime.SetFinalizer(c, func(c *Color) { C.coolbox_color_free(c.handle) })
	return c
}

type Canvas struct{ handle *C.CoolBoxCanvas }

func NewCanvas(width, height int) *Canvas {
	h := C.coolbox_canvas_create(C.int(width), C.int(height))
	c := &Canvas{handle: h}
	runtime.SetFinalizer(c, func(c *Canvas) { C.coolbox_canvas_free(c.handle) })
	return c
}

func (c *Canvas) SavePNG(path string) error {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	if C.coolbox_canvas_save_png(c.handle, cpath) != 0 {
		return fmt.Errorf("failed to save PNG")
	}
	return nil
}

func (c *Canvas) SaveBMP(path string) error {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	if C.coolbox_canvas_save_bmp(c.handle, cpath) != 0 {
		return fmt.Errorf("failed to save BMP")
	}
	return nil
}

func (c *Canvas) SaveJPG(path string, quality int) error {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	if C.coolbox_canvas_save_jpg(c.handle, cpath, C.int(quality)) != 0 {
		return fmt.Errorf("failed to save JPG")
	}
	return nil
}

type Graph struct{ handle *C.CoolBoxGraph }

func NewGraph(width, height, graphType int) *Graph {
	h := C.coolbox_graph_create(C.int(width), C.int(height), C.int(graphType))
	g := &Graph{handle: h}
	runtime.SetFinalizer(g, func(g *Graph) { C.coolbox_graph_free(g.handle) })
	return g
}

func (g *Graph) SetTitle(title string) {
	ctitle := C.CString(title)
	defer C.free(unsafe.Pointer(ctitle))
	C.coolbox_graph_set_title(g.handle, ctitle)
}

func (g *Graph) SetXLabel(label string) {
	clabel := C.CString(label)
	defer C.free(unsafe.Pointer(clabel))
	C.coolbox_graph_set_x_label(g.handle, clabel)
}

func (g *Graph) SetYLabel(label string) {
	clabel := C.CString(label)
	defer C.free(unsafe.Pointer(clabel))
	C.coolbox_graph_set_y_label(g.handle, clabel)
}

func (g *Graph) AddSeries(name string, x, y []float64, color *Color) error {
	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))
	n := len(x)
	if n != len(y) {
		return fmt.Errorf("x and y must have same length")
	}
	if n == 0 {
		return fmt.Errorf("series must contain at least one point")
	}
	if color == nil || color.handle == nil {
		return fmt.Errorf("color is nil")
	}
	if C.coolbox_graph_add_series(g.handle, cname, cDoublePtr(x), cDoublePtr(y), C.int(n), color.handle) != 0 {
		return fmt.Errorf("failed to add series")
	}
	return nil
}

func (g *Graph) Render() *Canvas { h := C.coolbox_graph_render(g.handle); return &Canvas{handle: h} }

type Table struct{ handle *C.CoolBoxTable }

func NewTable() *Table {
	h := C.coolbox_table_create()
	t := &Table{handle: h}
	runtime.SetFinalizer(t, func(t *Table) { C.coolbox_table_free(t.handle) })
	return t
}

func (t *Table) SetHeaders(headers []string) {
	cHeaders := make([]*C.char, len(headers))
	for i, s := range headers {
		cHeaders[i] = C.CString(s)
	}
	defer func() {
		for _, s := range cHeaders {
			C.free(unsafe.Pointer(s))
		}
	}()
	if len(cHeaders) == 0 {
		return
	}
	C.coolbox_table_set_headers(t.handle, &cHeaders[0], C.int(len(headers)))
}

func (t *Table) AddRow(row []string) {
	cRow := make([]*C.char, len(row))
	for i, s := range row {
		cRow[i] = C.CString(s)
	}
	defer func() {
		for _, s := range cRow {
			C.free(unsafe.Pointer(s))
		}
	}()
	if len(cRow) == 0 {
		return
	}
	C.coolbox_table_add_row(t.handle, &cRow[0], C.int(len(row)))
}

func (t *Table) Render() *Canvas { h := C.coolbox_table_render(t.handle); return &Canvas{handle: h} }

type FontFace struct{ handle *C.CoolBoxFontFace }

func NewFontFace() *FontFace {
	h := C.coolbox_fontface_create()
	f := &FontFace{handle: h}
	runtime.SetFinalizer(f, func(f *FontFace) { C.coolbox_fontface_free(f.handle) })
	return f
}

func (f *FontFace) LoadFromFile(path string) error {
	cpath := C.CString(path)
	defer C.free(unsafe.Pointer(cpath))
	if C.coolbox_fontface_load_from_file(f.handle, cpath) == 0 {
		return fmt.Errorf("failed to load font")
	}
	return nil
}

func (f *FontFace) IsLoaded() bool { return C.coolbox_fontface_is_loaded(f.handle) != 0 }

type Fractal struct{ handle *C.CoolBoxFractal }

func NewFractal(width, height, fractalType int) *Fractal {
	h := C.coolbox_fractal_create(C.int(width), C.int(height), C.int(fractalType))
	f := &Fractal{handle: h}
	runtime.SetFinalizer(f, func(f *Fractal) { C.coolbox_fractal_free(f.handle) })
	return f
}

func (f *Fractal) SetParams(param1, param2 float64) {
	C.coolbox_fractal_set_params(f.handle, C.double(param1), C.double(param2))
}

func (f *Fractal) SetMaxIter(maxIter int) { C.coolbox_fractal_set_max_iter(f.handle, C.int(maxIter)) }

func (f *Fractal) SetBounds(xMin, xMax, yMin, yMax float64) {
	C.coolbox_fractal_set_bounds(f.handle, C.double(xMin), C.double(xMax), C.double(yMin), C.double(yMax))
}

func (f *Fractal) Render() *Canvas {
	h := C.coolbox_fractal_render(f.handle)
	return &Canvas{handle: h}
}

type FunctionPlot struct{ handle *C.CoolBoxFunctionPlot }

func NewFunctionPlot(width, height int) *FunctionPlot {
	h := C.coolbox_function_plot_create(C.int(width), C.int(height))
	fp := &FunctionPlot{handle: h}
	runtime.SetFinalizer(fp, func(fp *FunctionPlot) { C.coolbox_function_plot_free(fp.handle) })
	return fp
}

func (fp *FunctionPlot) SetEquation(expr string) {
	cexpr := C.CString(expr)
	defer C.free(unsafe.Pointer(cexpr))
	C.coolbox_function_plot_set_equation(fp.handle, cexpr)
}

func (fp *FunctionPlot) SetRange(xMin, xMax float64) {
	C.coolbox_function_plot_set_range(fp.handle, C.double(xMin), C.double(xMax))
}

func (fp *FunctionPlot) SetSamples(n int) { C.coolbox_function_plot_set_samples(fp.handle, C.int(n)) }

func (fp *FunctionPlot) SetColor(color *Color) {
	C.coolbox_function_plot_set_color(fp.handle, color.handle)
}

func (fp *FunctionPlot) Render() *Canvas {
	h := C.coolbox_function_plot_render(fp.handle)
	return &Canvas{handle: h}
}

type ParametricPlot struct{ handle *C.CoolBoxParametricPlot }

func NewParametricPlot(width, height int) *ParametricPlot {
	h := C.coolbox_parametric_plot_create(C.int(width), C.int(height))
	pp := &ParametricPlot{handle: h}
	runtime.SetFinalizer(pp, func(pp *ParametricPlot) { C.coolbox_parametric_plot_free(pp.handle) })
	return pp
}

func (pp *ParametricPlot) SetEquations(xExpr, yExpr string) {
	cx := C.CString(xExpr)
	cy := C.CString(yExpr)
	defer C.free(unsafe.Pointer(cx))
	defer C.free(unsafe.Pointer(cy))
	C.coolbox_parametric_plot_set_equations(pp.handle, cx, cy)
}

func (pp *ParametricPlot) SetTRange(tMin, tMax float64) {
	C.coolbox_parametric_plot_set_t_range(pp.handle, C.double(tMin), C.double(tMax))
}

func (pp *ParametricPlot) SetSamples(n int) {
	C.coolbox_parametric_plot_set_samples(pp.handle, C.int(n))
}

func (pp *ParametricPlot) SetColor(color *Color) {
	C.coolbox_parametric_plot_set_color(pp.handle, color.handle)
}

func (pp *ParametricPlot) Render() *Canvas {
	h := C.coolbox_parametric_plot_render(pp.handle)
	return &Canvas{handle: h}
}

type PolarPlot struct{ handle *C.CoolBoxPolarPlot }

func NewPolarPlot(width, height int) *PolarPlot {
	h := C.coolbox_polar_plot_create(C.int(width), C.int(height))
	pp := &PolarPlot{handle: h}
	runtime.SetFinalizer(pp, func(pp *PolarPlot) { C.coolbox_polar_plot_free(pp.handle) })
	return pp
}

func (pp *PolarPlot) SetEquation(expr string) {
	cexpr := C.CString(expr)
	defer C.free(unsafe.Pointer(cexpr))
	C.coolbox_polar_plot_set_equation(pp.handle, cexpr)
}

func (pp *PolarPlot) SetThetaRange(thetaMin, thetaMax float64) {
	C.coolbox_polar_plot_set_theta_range(pp.handle, C.double(thetaMin), C.double(thetaMax))
}

func (pp *PolarPlot) SetSamples(n int)      { C.coolbox_polar_plot_set_samples(pp.handle, C.int(n)) }
func (pp *PolarPlot) SetColor(color *Color) { C.coolbox_polar_plot_set_color(pp.handle, color.handle) }

func (pp *PolarPlot) Render() *Canvas {
	h := C.coolbox_polar_plot_render(pp.handle)
	return &Canvas{handle: h}
}

type HistogramPlot struct{ handle *C.CoolBoxHistogramPlot }

func NewHistogramPlot(width, height int) *HistogramPlot {
	h := C.coolbox_histogram_plot_create(C.int(width), C.int(height))
	hp := &HistogramPlot{handle: h}
	runtime.SetFinalizer(hp, func(hp *HistogramPlot) { C.coolbox_histogram_plot_free(hp.handle) })
	return hp
}

func (hp *HistogramPlot) SetData(values []float64) {
	if len(values) == 0 {
		return
	}
	C.coolbox_histogram_plot_set_data(hp.handle, cDoublePtr(values), C.int(len(values)))
}

func (hp *HistogramPlot) SetBins(n int) { C.coolbox_histogram_plot_set_bins(hp.handle, C.int(n)) }

func (hp *HistogramPlot) SetColor(color *Color) {
	C.coolbox_histogram_plot_set_color(hp.handle, color.handle)
}

func (hp *HistogramPlot) Render() *Canvas {
	h := C.coolbox_histogram_plot_render(hp.handle)
	return &Canvas{handle: h}
}
