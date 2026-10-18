package coolbox

import (
	"strings"
	"testing"
)

func TestPlot2DAnd3DHelpers(t *testing.T) {
	t.Run("2D", func(t *testing.T) {
		canvas, err := Plot2D(
			[]float64{0, 1, 2},
			[]float64{2, 1, 3},
			Plot2DOptions{Width: 48, Height: 32},
		)
		if err != nil {
			t.Fatal(err)
		}
		defer canvas.Close()

		width, err := canvas.Width()
		if err != nil || width != 48 {
			t.Fatalf("Width() = %d, %v; want 48, nil", width, err)
		}
		height, err := canvas.Height()
		if err != nil || height != 32 {
			t.Fatalf("Height() = %d, %v; want 32, nil", height, err)
		}
		pixels, err := canvas.RGBA()
		if err != nil || len(pixels) != 48*32*4 {
			t.Fatalf("RGBA() returned %d bytes, %v; want %d bytes", len(pixels), err, 48*32*4)
		}
	})

	t.Run("3D", func(t *testing.T) {
		canvas, err := Plot3D(
			[]float64{0, 1, 2},
			[]float64{2, 1, 3},
			[]float64{1, 2, 0},
			Plot3DOptions{Width: 48, Height: 32},
		)
		if err != nil {
			t.Fatal(err)
		}
		defer canvas.Close()
		if width, err := canvas.Width(); err != nil || width != 48 {
			t.Fatalf("Width() = %d, %v; want 48, nil", width, err)
		}
	})
}

func TestPlotHelpersRejectInvalidInput(t *testing.T) {
	if _, err := Plot2D([]float64{1}, nil, Plot2DOptions{}); err == nil {
		t.Fatal("Plot2D accepted mismatched x/y values")
	}
	if _, err := Plot3D([]float64{1}, []float64{2}, nil, Plot3DOptions{}); err == nil {
		t.Fatal("Plot3D accepted mismatched x/y/z values")
	}
}

func TestPlotWindowReportsUnsupportedBuild(t *testing.T) {
	canvas, err := Plot2D([]float64{0, 1}, []float64{0, 1}, Plot2DOptions{Width: 32, Height: 24})
	if err != nil {
		t.Fatal(err)
	}
	defer canvas.Close()
	err = canvas.ShowWindow("Plot", "plot.png")
	if err == nil || !strings.Contains(err.Error(), "unavailable") {
		t.Fatalf("ShowWindow() error = %v; want an explicit unsupported-build error", err)
	}
}
