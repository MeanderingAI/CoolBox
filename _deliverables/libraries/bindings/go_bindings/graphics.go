package coolbox

/*
#include "graphics_bridge.h"
#include <stdlib.h>
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"strings"
	"unsafe"
)

func graphicsNativeError(buffer *[nativeErrorCapacity]C.char) error {
	return nativeError(&buffer[0], "graphics operation failed")
}

type GraphType int

const (
	LineGraph GraphType = iota
	BarGraph
	ScatterGraph
)

type Color struct {
	R uint8
	G uint8
	B uint8
	A uint8
}

var (
	Black     = Color{R: 0, G: 0, B: 0, A: 255}
	White     = Color{R: 255, G: 255, B: 255, A: 255}
	Red       = Color{R: 220, G: 50, B: 50, A: 255}
	Green     = Color{R: 50, G: 180, B: 50, A: 255}
	Blue      = Color{R: 50, G: 90, B: 220, A: 255}
	Orange    = Color{R: 230, G: 150, B: 30, A: 255}
	Purple    = Color{R: 150, G: 50, B: 200, A: 255}
	Cyan      = Color{R: 50, G: 200, B: 200, A: 255}
	Gray      = Color{R: 180, G: 180, B: 180, A: 255}
	DarkGray  = Color{R: 100, G: 100, B: 100, A: 255}
	LightGray = Color{R: 230, G: 230, B: 230, A: 255}
)

// Series contains a labeled set of x/y values rendered with one color.
type Series struct {
	Label string
	X     []float64
	Y     []float64
	Color Color
}

// Plot2DOptions configures the single-series plot helper.
type Plot2DOptions struct {
	Width       int
	Height      int
	Title       string
	XLabel      string
	YLabel      string
	SeriesLabel string
	Color       Color
	Type        GraphType
	ShowWindow  bool
	SavePath    string
}

// Plot3DOptions configures the single-series 3D plot helper.
type Plot3DOptions struct {
	Width            int
	Height           int
	Title            string
	XLabel           string
	YLabel           string
	ZLabel           string
	SeriesLabel      string
	Color            Color
	Type             Graph3DType
	AzimuthDegrees   float64
	ElevationDegrees float64
	ShowWindow       bool
	SavePath         string
}

// Graph3DType selects a line or scatter plot; the zero value in Plot3DOptions
// selects ScatterGraph3D.
type Graph3DType int

const (
	LineGraph3D Graph3DType = iota + 1
	ScatterGraph3D
)

// Graph builds a native line, bar, or scatter plot.
type Graph struct {
	handle unsafe.Pointer
	width  int
	height int
}

func NewGraph(width, height int, kind GraphType) (*Graph, error) {
	if width <= 0 || height <= 0 {
		return nil, errors.New("graph dimensions must be positive")
	}
	if width > math.MaxInt32 || height > math.MaxInt32 {
		return nil, errors.New("graph dimensions exceed the native integer range")
	}
	if kind < LineGraph || kind > ScatterGraph {
		return nil, errors.New("unknown graph type")
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_graph_new(C.int(width), C.int(height), C.int(kind),
		&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, graphicsNativeError(&nativeErr)
	}
	return &Graph{handle: handle, width: width, height: height}, nil
}

// Plot2D builds and renders a simple x/y plot. ShowWindow requests the native
// viewer and returns an unsupported-build error in the current Go bridge.
func Plot2D(x, y []float64, options Plot2DOptions) (*Canvas, error) {
	if len(x) == 0 || len(x) != len(y) {
		return nil, errors.New("plot x and y values must have equal, non-zero lengths")
	}
	options = normalizePlot2DOptions(options)
	if options.Width <= 0 || options.Height <= 0 ||
		options.Width > math.MaxInt32 || options.Height > math.MaxInt32 {
		return nil, errors.New("plot dimensions must be positive and fit the native integer range")
	}
	if options.Type < LineGraph || options.Type > ScatterGraph {
		return nil, errors.New("unknown graph type")
	}
	if err := validatePlotText(options.Title, options.XLabel, options.YLabel,
		options.SeriesLabel, options.SavePath); err != nil {
		return nil, err
	}
	color := options.Color
	title, xLabel, yLabel, seriesLabel, savePath := graphicsCString(options.Title),
		graphicsCString(options.XLabel), graphicsCString(options.YLabel),
		graphicsCString(options.SeriesLabel), graphicsCString(options.SavePath)
	defer C.free(unsafe.Pointer(title))
	defer C.free(unsafe.Pointer(xLabel))
	defer C.free(unsafe.Pointer(yLabel))
	defer C.free(unsafe.Pointer(seriesLabel))
	defer C.free(unsafe.Pointer(savePath))

	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_plot_2d(
		doublePointer(x), doublePointer(y), C.size_t(len(x)),
		title, xLabel, yLabel, seriesLabel,
		C.uint8_t(color.R), C.uint8_t(color.G), C.uint8_t(color.B), C.uint8_t(color.A),
		C.int(options.Width), C.int(options.Height), C.int(options.Type),
		&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, graphicsNativeError(&nativeErr)
	}
	canvas := &Canvas{handle: handle, width: options.Width, height: options.Height}
	if options.ShowWindow {
		if err := canvas.ShowWindow(options.Title, options.SavePath); err != nil {
			canvas.Close()
			return nil, err
		}
	}
	return canvas, nil
}

// Plot3D builds and renders a single-series projected 3D plot.
func Plot3D(x, y, z []float64, options Plot3DOptions) (*Canvas, error) {
	if len(x) == 0 || len(x) != len(y) || len(x) != len(z) {
		return nil, errors.New("plot x, y, and z values must have equal, non-zero lengths")
	}
	options = normalizePlot3DOptions(options)
	if options.Width <= 0 || options.Height <= 0 ||
		options.Width > math.MaxInt32 || options.Height > math.MaxInt32 {
		return nil, errors.New("plot dimensions must be positive and fit the native integer range")
	}
	if options.Type == 0 {
		options.Type = ScatterGraph3D
	}
	if options.Type < LineGraph3D || options.Type > ScatterGraph3D {
		return nil, errors.New("unknown 3D graph type")
	}
	if math.IsNaN(options.AzimuthDegrees) || math.IsInf(options.AzimuthDegrees, 0) ||
		math.IsNaN(options.ElevationDegrees) || math.IsInf(options.ElevationDegrees, 0) {
		return nil, errors.New("3D view angles must be finite")
	}
	if err := validatePlotText(options.Title, options.XLabel, options.YLabel,
		options.ZLabel, options.SeriesLabel, options.SavePath); err != nil {
		return nil, err
	}
	color := options.Color
	title, xLabel, yLabel, zLabel, seriesLabel, savePath := graphicsCString(options.Title),
		graphicsCString(options.XLabel), graphicsCString(options.YLabel), graphicsCString(options.ZLabel),
		graphicsCString(options.SeriesLabel), graphicsCString(options.SavePath)
	defer C.free(unsafe.Pointer(title))
	defer C.free(unsafe.Pointer(xLabel))
	defer C.free(unsafe.Pointer(yLabel))
	defer C.free(unsafe.Pointer(zLabel))
	defer C.free(unsafe.Pointer(seriesLabel))
	defer C.free(unsafe.Pointer(savePath))

	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_plot_3d(
		doublePointer(x), doublePointer(y), doublePointer(z), C.size_t(len(x)),
		title, xLabel, yLabel, zLabel, seriesLabel,
		C.uint8_t(color.R), C.uint8_t(color.G), C.uint8_t(color.B), C.uint8_t(color.A),
		C.int(options.Width), C.int(options.Height), C.int(options.Type-1),
		C.double(options.AzimuthDegrees), C.double(options.ElevationDegrees),
		&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, graphicsNativeError(&nativeErr)
	}
	canvas := &Canvas{handle: handle, width: options.Width, height: options.Height}
	if options.ShowWindow {
		if err := canvas.ShowWindow(options.Title, options.SavePath); err != nil {
			canvas.Close()
			return nil, err
		}
	}
	return canvas, nil
}

func normalizePlot2DOptions(options Plot2DOptions) Plot2DOptions {
	if options.Width == 0 {
		options.Width = 800
	}
	if options.Height == 0 {
		options.Height = 600
	}
	if options.XLabel == "" {
		options.XLabel = "X"
	}
	if options.YLabel == "" {
		options.YLabel = "Y"
	}
	if options.SeriesLabel == "" {
		options.SeriesLabel = "Series"
	}
	if options.Color == (Color{}) {
		options.Color = Blue
	}
	if options.SavePath == "" {
		options.SavePath = "plot.png"
	}
	return options
}

func normalizePlot3DOptions(options Plot3DOptions) Plot3DOptions {
	if options.Width == 0 {
		options.Width = 800
	}
	if options.Height == 0 {
		options.Height = 600
	}
	if options.XLabel == "" {
		options.XLabel = "X"
	}
	if options.YLabel == "" {
		options.YLabel = "Y"
	}
	if options.ZLabel == "" {
		options.ZLabel = "Z"
	}
	if options.SeriesLabel == "" {
		options.SeriesLabel = "Series"
	}
	if options.Color == (Color{}) {
		options.Color = Blue
	}
	if options.AzimuthDegrees == 0 {
		options.AzimuthDegrees = 45
	}
	if options.ElevationDegrees == 0 {
		options.ElevationDegrees = 25
	}
	if options.SavePath == "" {
		options.SavePath = "plot.png"
	}
	return options
}

func validatePlotText(values ...string) error {
	for _, value := range values {
		if strings.IndexByte(value, 0) >= 0 {
			return errors.New("plot text and paths must not contain a NUL byte")
		}
	}
	return nil
}

func graphicsCString(value string) *C.char {
	return C.CString(value)
}

func (g *Graph) valid() error {
	if g == nil || g.handle == nil {
		return errors.New("graph is closed")
	}
	return nil
}

func (g *Graph) Close() error {
	if g != nil && g.handle != nil {
		C.coolbox_graph_free(g.handle)
		g.handle = nil
	}
	return nil
}

func (g *Graph) SetTitle(title string) error {
	return g.setText(title, 0)
}

func (g *Graph) SetXLabel(label string) error {
	return g.setText(label, 1)
}

func (g *Graph) SetYLabel(label string) error {
	return g.setText(label, 2)
}

func (g *Graph) setText(value string, field int) error {
	if err := g.valid(); err != nil {
		return err
	}
	if strings.IndexByte(value, 0) >= 0 {
		return errors.New("graph text must not contain a NUL byte")
	}
	text := C.CString(value)
	defer C.free(unsafe.Pointer(text))
	var nativeErr [nativeErrorCapacity]C.char
	var result C.int
	switch field {
	case 0:
		result = C.coolbox_graph_set_title(g.handle, text, &nativeErr[0], C.size_t(len(nativeErr)))
	case 1:
		result = C.coolbox_graph_set_x_label(g.handle, text, &nativeErr[0], C.size_t(len(nativeErr)))
	case 2:
		result = C.coolbox_graph_set_y_label(g.handle, text, &nativeErr[0], C.size_t(len(nativeErr)))
	default:
		return errors.New("unknown graph text field")
	}
	if result == 0 {
		return graphicsNativeError(&nativeErr)
	}
	return nil
}

func (g *Graph) AddSeries(series Series) error {
	if err := g.valid(); err != nil {
		return err
	}
	if len(series.X) == 0 || len(series.X) != len(series.Y) {
		return errors.New("series x and y values must have equal, non-zero lengths")
	}
	if strings.IndexByte(series.Label, 0) >= 0 {
		return errors.New("series label must not contain a NUL byte")
	}
	if len(series.Label) > 0 && len(series.Label) > math.MaxInt32 {
		return errors.New("series label is too long")
	}
	for i := range series.X {
		if math.IsNaN(series.X[i]) || math.IsInf(series.X[i], 0) ||
			math.IsNaN(series.Y[i]) || math.IsInf(series.Y[i], 0) {
			return fmt.Errorf("series value %d is not finite", i)
		}
	}
	label := C.CString(series.Label)
	defer C.free(unsafe.Pointer(label))
	color := series.Color
	if color == (Color{}) {
		color = Blue
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_graph_add_series(g.handle, label, doublePointer(series.X), doublePointer(series.Y),
		C.size_t(len(series.X)), C.uint8_t(color.R), C.uint8_t(color.G),
		C.uint8_t(color.B), C.uint8_t(color.A), &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return graphicsNativeError(&nativeErr)
	}
	return nil
}

func (g *Graph) Render() (*Canvas, error) {
	if err := g.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_graph_render(g.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, graphicsNativeError(&nativeErr)
	}
	return &Canvas{handle: handle, width: g.width, height: g.height}, nil
}

func (g *Graph) SaveBMP(path string) error              { return g.save(path, 0, 0) }
func (g *Graph) SavePNG(path string) error              { return g.save(path, 1, 0) }
func (g *Graph) SaveJPG(path string, quality int) error { return g.save(path, 2, quality) }

func (g *Graph) save(path string, format, quality int) error {
	canvas, err := g.Render()
	if err != nil {
		return err
	}
	defer canvas.Close()
	return canvas.save(path, format, quality)
}

// Canvas is a rendered RGBA image backed by the native charting library.
type Canvas struct {
	handle unsafe.Pointer
	width  int
	height int
}

func (c *Canvas) valid() error {
	if c == nil || c.handle == nil {
		return errors.New("canvas is closed")
	}
	return nil
}

func (c *Canvas) Close() error {
	if c != nil && c.handle != nil {
		C.coolbox_canvas_free(c.handle)
		c.handle = nil
	}
	return nil
}

func (c *Canvas) Width() (int, error) {
	if err := c.valid(); err != nil {
		return 0, err
	}
	return int(C.coolbox_canvas_width(c.handle)), nil
}

func (c *Canvas) Height() (int, error) {
	if err := c.valid(); err != nil {
		return 0, err
	}
	return int(C.coolbox_canvas_height(c.handle)), nil
}

// RGBA returns an independent copy of the rendered image in row-major RGBA order.
func (c *Canvas) RGBA() ([]byte, error) {
	if err := c.valid(); err != nil {
		return nil, err
	}
	width := int(C.coolbox_canvas_width(c.handle))
	height := int(C.coolbox_canvas_height(c.handle))
	if width <= 0 || height <= 0 || width > math.MaxInt/4/height {
		return nil, errors.New("native canvas dimensions are invalid")
	}
	pixels := make([]byte, width*height*4)
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_canvas_copy_data(c.handle, (*C.uint8_t)(unsafe.Pointer(&pixels[0])),
		C.size_t(len(pixels)), &nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return nil, graphicsNativeError(&nativeErr)
	}
	return pixels, nil
}

func (c *Canvas) SaveBMP(path string) error              { return c.save(path, 0, 0) }
func (c *Canvas) SavePNG(path string) error              { return c.save(path, 1, 0) }
func (c *Canvas) SaveJPG(path string, quality int) error { return c.save(path, 2, quality) }

// ShowWindow requests the native viewer; the current Go bridge reports that
// interactive windows are unavailable in its build.
func (c *Canvas) ShowWindow(title, savePath string) error {
	if err := c.valid(); err != nil {
		return err
	}
	if savePath == "" {
		return errors.New("save path must not be empty")
	}
	if strings.IndexByte(savePath, 0) >= 0 || strings.IndexByte(title, 0) >= 0 {
		return errors.New("window title and save path must not contain NUL bytes")
	}
	nativeTitle := C.CString(title)
	nativePath := C.CString(savePath)
	defer C.free(unsafe.Pointer(nativeTitle))
	defer C.free(unsafe.Pointer(nativePath))
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_canvas_show_window(c.handle, nativeTitle, nativePath,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return graphicsNativeError(&nativeErr)
	}
	return nil
}

func (c *Canvas) save(path string, format, quality int) error {
	if err := c.valid(); err != nil {
		return err
	}
	if path == "" {
		return errors.New("output path must not be empty")
	}
	if strings.IndexByte(path, 0) >= 0 {
		return errors.New("output path must not contain a NUL byte")
	}
	nativePath := C.CString(path)
	defer C.free(unsafe.Pointer(nativePath))
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_canvas_save(c.handle, nativePath, C.int(format), C.int(quality),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return graphicsNativeError(&nativeErr)
	}
	return nil
}
