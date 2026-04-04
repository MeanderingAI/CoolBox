# Detect outliers in a CoolBox time series

Detect outliers in a CoolBox time series

## Usage

``` r
coolbox_time_series_outliers(
  values,
  method = c("zscore", "iqr"),
  threshold = 3,
  multiplier = 1.5
)
```

## Arguments

- values:

  Numeric vector of observations.

- method:

  Outlier detection method.

- threshold:

  Z-score threshold when `method = "zscore"`.

- multiplier:

  IQR multiplier when `method = "iqr"`.

## Value

Integer indices of detected outliers using R's 1-based indexing.
