# Transform a CoolBox time series

Transform a CoolBox time series

## Usage

``` r
coolbox_time_series_transform(
  values,
  method = c("normalize", "min_max_scale", "diff", "log_transform", "moving_average",
    "exponential_smoothing", "resample"),
  lag = 1L,
  window_size = 3L,
  alpha = 0.3,
  new_size = length(values),
  min_val = 0,
  max_val = 1
)
```

## Arguments

- values:

  Numeric vector of observations.

- method:

  Transformation method.

- lag:

  Lag for differencing.

- window_size:

  Window size for moving averages.

- alpha:

  Smoothing factor for exponential smoothing.

- new_size:

  Target size for resampling.

- min_val:

  Minimum value for min-max scaling.

- max_val:

  Maximum value for min-max scaling.

## Value

A numeric vector containing the transformed time series.
