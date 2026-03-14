test_that("coolbox time series summary exposes native statistics", {
  values <- c(1, 2, 4, 8, 16)

  summary <- coolbox_time_series_summary(values, max_lag = 3)

  expect_s3_class(summary, "coolbox_time_series_summary")
  expect_equal(summary$length, length(values))
  expect_equal(summary$mean, mean(values))
  expect_equal(summary$median, median(values))
  expect_length(summary$autocorrelation, 3)
})

test_that("coolbox time series transforms run through native code", {
  values <- c(1, 2, 4, 8, 16)

  normalized <- coolbox_time_series_transform(values, method = "normalize")
  differenced <- coolbox_time_series_transform(values, method = "diff", lag = 1)
  moving_avg <- coolbox_time_series_transform(values, method = "moving_average", window_size = 2)

  expect_length(normalized, length(values))
  expect_equal(length(differenced), length(values) - 1L)
  expect_equal(moving_avg, c(1.5, 3, 6, 12), tolerance = 1e-8)
})

test_that("coolbox time series forecasting and outlier helpers work", {
  values <- c(10, 12, 11, 13, 12, 14, 40)

  ma_forecast <- coolbox_time_series_forecast(values, method = "moving_average", steps = 2, window_size = 3)
  ar_forecast <- coolbox_time_series_forecast(values, method = "autoregressive", steps = 2, order = 2)
  outliers <- coolbox_time_series_outliers(values, method = "zscore", threshold = 1.8)

  expect_length(ma_forecast, 2)
  expect_length(ar_forecast, 2)
  expect_true(7L %in% outliers)
})