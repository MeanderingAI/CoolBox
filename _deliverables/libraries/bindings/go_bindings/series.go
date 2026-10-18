package coolbox

/*
#include <stdlib.h>
void* coolbox_series_new(const double* values, size_t count);
void coolbox_series_free(void* handle);
size_t coolbox_series_size(const void* handle);
size_t coolbox_series_autocorrelation_size(const void* handle, size_t max_lag);
int coolbox_series_copy_values(const void* handle, double* destination, size_t capacity);
int coolbox_series_statistic(const void* handle, int statistic, double* result);
void* coolbox_series_transform(void* handle, int operation, size_t integer_argument, double first_argument, double second_argument, char* error, size_t error_capacity);
int coolbox_series_copy_autocorrelation(const void* handle, size_t max_lag, double* destination, size_t capacity, char* error, size_t error_capacity);
*/
import "C"

import (
	"errors"
	"unsafe"
)

// TimeSeries wraps the native univariate time-series utilities.
type TimeSeries struct {
	handle unsafe.Pointer
}

// NewTimeSeries creates a time series from values.
func NewTimeSeries(values []float64) (*TimeSeries, error) {
	var data *C.double
	if len(values) != 0 {
		data = (*C.double)(unsafe.Pointer(&values[0]))
	}
	handle := C.coolbox_series_new(data, C.size_t(len(values)))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create time series")
	}
	return &TimeSeries{handle: handle}, nil
}

// Close releases the native time-series object.
func (series *TimeSeries) Close() error {
	if series == nil || series.handle == nil {
		return nil
	}
	C.coolbox_series_free(series.handle)
	series.handle = nil
	return nil
}

// Values returns a copy of the observations.
func (series *TimeSeries) Values() ([]float64, error) {
	handle, err := series.nativeHandle()
	if err != nil {
		return nil, err
	}
	size := int(C.coolbox_series_size(handle))
	values := make([]C.double, size)
	if size != 0 && C.coolbox_series_copy_values(handle, &values[0], C.size_t(size)) == 0 {
		return nil, errors.New("coolbox: failed to read time-series values")
	}
	result := make([]float64, size)
	for index, value := range values {
		result[index] = float64(value)
	}
	return result, nil
}

// Size returns the number of observations.
func (series *TimeSeries) Size() (uint, error) {
	handle, err := series.nativeHandle()
	if err != nil {
		return 0, err
	}
	return uint(C.coolbox_series_size(handle)), nil
}

// Mean returns the arithmetic mean.
func (series *TimeSeries) Mean() (float64, error) { return series.statistic(0) }

// Std returns the sample standard deviation.
func (series *TimeSeries) Std() (float64, error) { return series.statistic(1) }

// Min returns the minimum observation.
func (series *TimeSeries) Min() (float64, error) { return series.statistic(2) }

// Max returns the maximum observation.
func (series *TimeSeries) Max() (float64, error) { return series.statistic(3) }

// Median returns the median observation.
func (series *TimeSeries) Median() (float64, error) { return series.statistic(4) }

// Normalize returns a z-score normalized copy.
func (series *TimeSeries) Normalize() (*TimeSeries, error) { return series.transform(0, 0, 0, 0) }

// MinMaxScale maps values into [minimum, maximum].
func (series *TimeSeries) MinMaxScale(minimum, maximum float64) (*TimeSeries, error) {
	return series.transform(1, 0, minimum, maximum)
}

// Diff returns the lagged difference series.
func (series *TimeSeries) Diff(lag uint) (*TimeSeries, error) {
	return series.transform(2, lag, 0, 0)
}

// LogTransform applies the native natural-log transform.
func (series *TimeSeries) LogTransform() (*TimeSeries, error) {
	return series.transform(3, 0, 0, 0)
}

// MovingAverage returns a trailing moving-average series.
func (series *TimeSeries) MovingAverage(window uint) (*TimeSeries, error) {
	return series.transform(4, window, 0, 0)
}

// ExponentialSmoothing returns an exponentially smoothed copy.
func (series *TimeSeries) ExponentialSmoothing(alpha float64) (*TimeSeries, error) {
	return series.transform(5, 0, alpha, 0)
}

// Resample returns a linearly interpolated series with size observations.
func (series *TimeSeries) Resample(size uint) (*TimeSeries, error) {
	return series.transform(6, size, 0, 0)
}

// Autocorrelation returns autocorrelation values for lags 0 through maxLag.
func (series *TimeSeries) Autocorrelation(maxLag uint) ([]float64, error) {
	handle, err := series.nativeHandle()
	if err != nil {
		return nil, err
	}
	if maxLag == ^uint(0) {
		maxLag--
	}
	count := int(C.coolbox_series_autocorrelation_size(handle, C.size_t(maxLag)))
	if count == 0 {
		return []float64{}, nil
	}
	values := make([]C.double, count)
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_series_copy_autocorrelation(
		handle,
		C.size_t(count-1),
		&values[0],
		C.size_t(len(values)),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nil, nativeError(errorBuffer, "failed to calculate autocorrelation")
	}
	result := make([]float64, len(values))
	for index, value := range values {
		result[index] = float64(value)
	}
	return result, nil
}

// CreateWindows returns fixed-width sliding observation windows.
func (series *TimeSeries) CreateWindows(windowSize, stride uint) ([][]float64, error) {
	if stride == 0 {
		return nil, errors.New("coolbox: window stride must be positive")
	}
	values, err := series.Values()
	if err != nil {
		return nil, err
	}
	if windowSize == 0 || windowSize > uint(len(values)) {
		return [][]float64{}, nil
	}
	windows := make([][]float64, 0, (uint(len(values))-windowSize)/stride+1)
	lastStart := uint(len(values)) - windowSize
	for start := uint(0); start <= lastStart; {
		window := append([]float64(nil), values[start:start+windowSize]...)
		windows = append(windows, window)
		if stride > lastStart-start {
			break
		}
		start += stride
	}
	return windows, nil
}

func (series *TimeSeries) statistic(statistic C.int) (float64, error) {
	handle, err := series.nativeHandle()
	if err != nil {
		return 0, err
	}
	var result C.double
	if C.coolbox_series_statistic(handle, statistic, &result) == 0 {
		return 0, errors.New("coolbox: failed to calculate time-series statistic")
	}
	return float64(result), nil
}

func (series *TimeSeries) transform(operation C.int, integerArgument uint, first, second float64) (*TimeSeries, error) {
	handle, err := series.nativeHandle()
	if err != nil {
		return nil, err
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	transformed := C.coolbox_series_transform(
		handle,
		operation,
		C.size_t(integerArgument),
		C.double(first),
		C.double(second),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if transformed == nil {
		return nil, nativeError(errorBuffer, "failed to transform time series")
	}
	return &TimeSeries{handle: transformed}, nil
}

func (series *TimeSeries) nativeHandle() (unsafe.Pointer, error) {
	if series == nil || series.handle == nil {
		return nil, errors.New("coolbox: time series is closed")
	}
	return series.handle, nil
}
