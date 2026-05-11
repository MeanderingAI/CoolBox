#' Summarize a CoolBox time series
#'
#' Compute descriptive statistics and autocorrelation values using the native
#' CoolBox time-series implementation.
#'
#' @param values Numeric vector of observations.
#' @param max_lag Maximum autocorrelation lag to compute.
#'
#' @return An object of class `coolbox_time_series_summary`.
#' @export
coolbox_time_series_summary <- function(values, max_lag = min(10L, max(length(values) - 1L, 0L))) {
  values <- as.numeric(values)

  if (!length(values)) {
    stop("`values` must contain at least one observation.", call. = FALSE)
  }

  if (any(!is.finite(values))) {
    stop("`values` must be finite numeric values.", call. = FALSE)
  }

  if (length(max_lag) != 1L || !is.finite(max_lag) || max_lag < 0) {
    stop("`max_lag` must be a single non-negative integer.", call. = FALSE)
  }

  summary <- .Call(`_coolboxr_time_series_summary`, values, as.integer(max_lag))
  structure(summary, class = "coolbox_time_series_summary")
}

#' @export
print.coolbox_time_series_summary <- function(x, ...) {
  cat("CoolBox time-series summary\n")
  cat("  length:", x$length, "\n")
  cat("  mean:", signif(x$mean, 6), "\n")
  cat("  sd:", signif(x$std, 6), "\n")
  cat("  min:", signif(x$min, 6), "\n")
  cat("  median:", signif(x$median, 6), "\n")
  cat("  max:", signif(x$max, 6), "\n")
  invisible(x)
}

#' Transform a CoolBox time series
#'
#' @param values Numeric vector of observations.
#' @param method Transformation method.
#' @param lag Lag for differencing.
#' @param window_size Window size for moving averages.
#' @param alpha Smoothing factor for exponential smoothing.
#' @param new_size Target size for resampling.
#' @param min_val Minimum value for min-max scaling.
#' @param max_val Maximum value for min-max scaling.
#'
#' @return A numeric vector containing the transformed time series.
#' @export
coolbox_time_series_transform <- function(
  values,
  method = c(
    "normalize",
    "min_max_scale",
    "diff",
    "log_transform",
    "moving_average",
    "exponential_smoothing",
    "resample"
  ),
  lag = 1L,
  window_size = 3L,
  alpha = 0.3,
  new_size = length(values),
  min_val = 0,
  max_val = 1
) {
  method <- match.arg(method)
  values <- as.numeric(values)

  if (!length(values)) {
    stop("`values` must contain at least one observation.", call. = FALSE)
  }

  if (any(!is.finite(values))) {
    stop("`values` must be finite numeric values.", call. = FALSE)
  }

  primary <- switch(
    method,
    normalize = 0,
    min_max_scale = min_val,
    diff = as.integer(lag),
    log_transform = 0,
    moving_average = as.integer(window_size),
    exponential_smoothing = alpha,
    resample = as.integer(new_size)
  )

  secondary <- switch(
    method,
    min_max_scale = max_val,
    0
  )

  transformed <- .Call(
    `_coolboxr_transform_time_series`,
    values,
    method,
    as.numeric(primary),
    as.numeric(secondary),
    0
  )

  as.numeric(transformed)
}

#' Forecast a CoolBox time series
#'
#' @param values Numeric vector of observations.
#' @param method Forecasting method.
#' @param steps Number of future steps to forecast.
#' @param window_size Window size for moving-average forecasting.
#' @param alpha Level smoothing parameter.
#' @param beta Trend smoothing parameter.
#' @param gamma Seasonal smoothing parameter.
#' @param order Autoregressive order.
#'
#' @return A numeric vector of forecasts.
#' @export
coolbox_time_series_forecast <- function(
  values,
  method = c("moving_average", "exponential_smoothing", "autoregressive"),
  steps = 1L,
  window_size = 3L,
  alpha = 0.3,
  beta = 0.1,
  gamma = 0,
  order = 2L
) {
  method <- match.arg(method)
  values <- as.numeric(values)

  if (!length(values)) {
    stop("`values` must contain at least one observation.", call. = FALSE)
  }

  if (any(!is.finite(values))) {
    stop("`values` must be finite numeric values.", call. = FALSE)
  }

  if (length(steps) != 1L || !is.finite(steps) || steps < 1) {
    stop("`steps` must be a single positive integer.", call. = FALSE)
  }

  primary <- switch(
    method,
    moving_average = as.integer(window_size),
    exponential_smoothing = alpha,
    autoregressive = as.integer(order)
  )

  secondary <- switch(
    method,
    exponential_smoothing = beta,
    0
  )

  tertiary <- switch(
    method,
    exponential_smoothing = gamma,
    0
  )

  forecast <- .Call(
    `_coolboxr_forecast_time_series`,
    values,
    method,
    as.integer(steps),
    as.numeric(primary),
    as.numeric(secondary),
    as.numeric(tertiary)
  )

  as.numeric(forecast)
}

#' Detect outliers in a CoolBox time series
#'
#' @param values Numeric vector of observations.
#' @param method Outlier detection method.
#' @param threshold Z-score threshold when `method = "zscore"`.
#' @param multiplier IQR multiplier when `method = "iqr"`.
#'
#' @return Integer indices of detected outliers using R's 1-based indexing.
#' @export
coolbox_time_series_outliers <- function(
  values,
  method = c("zscore", "iqr"),
  threshold = 3,
  multiplier = 1.5
) {
  method <- match.arg(method)
  values <- as.numeric(values)

  if (!length(values)) {
    stop("`values` must contain at least one observation.", call. = FALSE)
  }

  if (any(!is.finite(values))) {
    stop("`values` must be finite numeric values.", call. = FALSE)
  }

  parameter <- if (identical(method, "zscore")) threshold else multiplier
  outliers <- .Call(`_coolboxr_detect_time_series_outliers`, values, method, as.numeric(parameter))
  as.integer(outliers)
}