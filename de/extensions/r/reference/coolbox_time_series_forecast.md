# Forecast a CoolBox time series

Forecast a CoolBox time series

## Usage

``` r
coolbox_time_series_forecast(
  values,
  method = c("moving_average", "exponential_smoothing", "autoregressive"),
  steps = 1L,
  window_size = 3L,
  alpha = 0.3,
  beta = 0.1,
  gamma = 0,
  order = 2L
)
```

## Arguments

- values:

  Numeric vector of observations.

- method:

  Forecasting method.

- steps:

  Number of future steps to forecast.

- window_size:

  Window size for moving-average forecasting.

- alpha:

  Level smoothing parameter.

- beta:

  Trend smoothing parameter.

- gamma:

  Seasonal smoothing parameter.

- order:

  Autoregressive order.

## Value

A numeric vector of forecasts.
