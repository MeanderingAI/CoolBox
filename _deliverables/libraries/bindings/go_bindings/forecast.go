package coolbox

/*
#include <stdlib.h>
void* coolbox_forecaster_new(int kind, size_t first_order, size_t second_order, size_t third_order, double alpha, double beta, double gamma);
void coolbox_forecaster_free(void* handle);
int coolbox_forecaster_fit(void* handle, const double* values, size_t count, char* error, size_t error_capacity);
int coolbox_forecaster_forecast(void* handle, size_t steps, double* destination, size_t capacity, char* error, size_t error_capacity);
int coolbox_moving_average_forecast_one_step(void* handle, double* prediction, char* error, size_t error_capacity);
*/
import "C"

import (
	"errors"
	"math"
	"unsafe"
)

// Forecaster is a native time-series forecasting model.
type Forecaster struct {
	handle unsafe.Pointer
}

// MovingAverageForecaster forecasts using a trailing moving average.
type MovingAverageForecaster struct{ Forecaster }

// ExponentialSmoothingForecaster forecasts with smoothed level and trend.
type ExponentialSmoothingForecaster struct{ Forecaster }

// AutoRegressiveModel forecasts using an autoregressive model.
type AutoRegressiveModel struct{ Forecaster }

// ARMAModel forecasts using an ARMA model.
type ARMAModel struct{ Forecaster }

// ARIMAModel forecasts using an ARIMA model.
type ARIMAModel struct{ Forecaster }

// NewMovingAverageForecaster creates a moving-average forecaster.
func NewMovingAverageForecaster(windowSize uint) (*MovingAverageForecaster, error) {
	if windowSize == 0 {
		return nil, errors.New("coolbox: moving-average window size must be positive")
	}
	model, err := newForecaster(0, windowSize, 0, 0, 0, 0, 0)
	if err != nil {
		return nil, err
	}
	return &MovingAverageForecaster{Forecaster: *model}, nil
}

// NewExponentialSmoothingForecaster creates an exponential-smoothing forecaster.
func NewExponentialSmoothingForecaster(alpha, beta, gamma float64) (*ExponentialSmoothingForecaster, error) {
	if !finite(alpha) || !finite(beta) || !finite(gamma) {
		return nil, errors.New("coolbox: smoothing parameters must be finite")
	}
	model, err := newForecaster(1, 0, 0, 0, alpha, beta, gamma)
	if err != nil {
		return nil, err
	}
	return &ExponentialSmoothingForecaster{Forecaster: *model}, nil
}

// NewAutoRegressiveModel creates an autoregressive forecaster.
func NewAutoRegressiveModel(order uint) (*AutoRegressiveModel, error) {
	model, err := newForecaster(2, order, 0, 0, 0, 0, 0)
	if err != nil {
		return nil, err
	}
	return &AutoRegressiveModel{Forecaster: *model}, nil
}

// NewARMAModel creates an ARMA forecaster with p AR and q MA coefficients.
func NewARMAModel(p, q uint) (*ARMAModel, error) {
	model, err := newForecaster(3, p, q, 0, 0, 0, 0)
	if err != nil {
		return nil, err
	}
	return &ARMAModel{Forecaster: *model}, nil
}

// NewARIMAModel creates an ARIMA forecaster with p, d, and q orders.
func NewARIMAModel(p, d, q uint) (*ARIMAModel, error) {
	model, err := newForecaster(4, p, d, q, 0, 0, 0)
	if err != nil {
		return nil, err
	}
	return &ARIMAModel{Forecaster: *model}, nil
}

func newForecaster(kind int, first, second, third uint, alpha, beta, gamma float64) (*Forecaster, error) {
	handle := C.coolbox_forecaster_new(
		C.int(kind),
		C.size_t(first),
		C.size_t(second),
		C.size_t(third),
		C.double(alpha),
		C.double(beta),
		C.double(gamma),
	)
	if handle == nil {
		return nil, errors.New("coolbox: failed to create forecaster")
	}
	return &Forecaster{handle: handle}, nil
}

// Close releases the native forecasting model.
func (model *Forecaster) Close() error {
	if model == nil || model.handle == nil {
		return nil
	}
	C.coolbox_forecaster_free(model.handle)
	model.handle = nil
	return nil
}

// Fit trains the forecaster using the provided observations.
func (model *Forecaster) Fit(values []float64) error {
	handle, err := model.nativeHandle()
	if err != nil {
		return err
	}
	if len(values) == 0 {
		return errors.New("coolbox: observations must not be empty")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_forecaster_fit(
		handle,
		(*C.double)(unsafe.Pointer(&values[0])),
		C.size_t(len(values)),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nativeError(errorBuffer, "failed to fit forecaster")
	}
	return nil
}

// Forecast predicts the next steps observations.
func (model *Forecaster) Forecast(steps uint) ([]float64, error) {
	handle, err := model.nativeHandle()
	if err != nil {
		return nil, err
	}
	if uint64(steps) > uint64(maxInt()) {
		return nil, errors.New("coolbox: forecast length is too large")
	}
	predictions := make([]C.double, int(steps))
	var destination *C.double
	if len(predictions) != 0 {
		destination = &predictions[0]
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_forecaster_forecast(
		handle,
		C.size_t(steps),
		destination,
		C.size_t(len(predictions)),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nil, nativeError(errorBuffer, "failed to forecast time series")
	}
	result := make([]float64, len(predictions))
	for index, prediction := range predictions {
		result[index] = float64(prediction)
	}
	return result, nil
}

// ForecastOneStep predicts one step using the moving-average model.
func (model *MovingAverageForecaster) ForecastOneStep() (float64, error) {
	handle, err := model.nativeHandle()
	if err != nil {
		return 0, err
	}
	var prediction C.double
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_moving_average_forecast_one_step(
		handle,
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return 0, nativeError(errorBuffer, "failed to forecast one step")
	}
	return float64(prediction), nil
}

func (model *Forecaster) nativeHandle() (unsafe.Pointer, error) {
	if model == nil || model.handle == nil {
		return nil, errors.New("coolbox: forecaster is closed")
	}
	return model.handle, nil
}

func finite(value float64) bool {
	return !math.IsNaN(value) && !math.IsInf(value, 0)
}

func maxInt() int {
	return int(^uint(0) >> 1)
}
