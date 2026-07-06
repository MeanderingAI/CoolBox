#' Fit CoolBox linear regression
#'
#' Train a small linear regression model backed by the native CoolBox C++
#' implementation.
#'
#' @param x Numeric matrix or data frame of predictors.
#' @param y Numeric response vector.
#' @param method Fitting method. Either `"closed_form"` or
#'   `"gradient_descent"`.
#' @param iterations Number of gradient descent iterations.
#'   Ignored for the closed-form solver.
#' @param learning_rate Gradient descent learning rate.
#'
#' @return An object of class `coolbox_linear_regression`.
#' @export
#'
#' @examples
#' x <- matrix(c(1, 2, 2, 0, 3, 1, 4, 3), ncol = 2, byrow = TRUE)
#' y <- c(3, 2, 5, 8)
#' fit <- coolbox_linear_regression(x, y)
#' predict(fit, x)
coolbox_linear_regression <- function(
  x,
  y,
  method = c("closed_form", "gradient_descent"),
  iterations = 1000L,
  learning_rate = 0.01
) {
  method <- match.arg(method)
  x <- as.matrix(x)
  storage.mode(x) <- "double"
  y <- as.numeric(y)

  if (!is.numeric(x) || !is.matrix(x)) {
    stop("`x` must be coercible to a numeric matrix.", call. = FALSE)
  }

  if (length(y) != nrow(x)) {
    stop("`y` must have the same number of rows as `x`.", call. = FALSE)
  }

  if (length(iterations) != 1L || !is.finite(iterations) || iterations < 1) {
    stop("`iterations` must be a single positive integer.", call. = FALSE)
  }

  if (length(learning_rate) != 1L || !is.finite(learning_rate) || learning_rate <= 0) {
    stop("`learning_rate` must be a single positive number.", call. = FALSE)
  }

  model <- .Call(
    `_coolboxr_fit_linear_regression`,
    x,
    y,
    method,
    as.integer(iterations),
    as.numeric(learning_rate)
  )

  structure(model, class = "coolbox_linear_regression")
}

#' Predict with a CoolBox linear regression model
#'
#' @param object A `coolbox_linear_regression` model.
#' @param newdata Numeric matrix or data frame with the same number of columns
#'   used during training.
#' @param ... Unused.
#'
#' @return A numeric vector of predictions.
#' @export
predict.coolbox_linear_regression <- function(object, newdata, ...) {
  newdata <- as.matrix(newdata)
  storage.mode(newdata) <- "double"
  .Call(`_coolboxr_predict_linear_regression`, object, newdata)
}

#' @export
print.coolbox_linear_regression <- function(x, ...) {
  cat("CoolBox linear regression\n")
  cat("  method:", x$method, "\n")
  cat("  intercept:", signif(x$intercept, 6), "\n")
  cat("  weights:", paste(signif(x$weights, 6), collapse = ", "), "\n")
  invisible(x)
}
