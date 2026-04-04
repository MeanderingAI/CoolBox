# Summarize a CoolBox time series

Compute descriptive statistics and autocorrelation values using the
native CoolBox time-series implementation.

## Usage

``` r
coolbox_time_series_summary(
  values,
  max_lag = min(10L, max(length(values) - 1L, 0L))
)
```

## Arguments

- values:

  Numeric vector of observations.

- max_lag:

  Maximum autocorrelation lag to compute.

## Value

An object of class `coolbox_time_series_summary`.
