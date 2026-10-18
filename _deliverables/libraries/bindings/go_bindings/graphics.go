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
