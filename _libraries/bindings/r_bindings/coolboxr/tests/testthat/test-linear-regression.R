test_that("coolbox linear regression fits and predicts", {
  x <- matrix(c(
    1, 1,
    2, 0,
    3, 1,
    4, 3
  ), ncol = 2, byrow = TRUE)
  y <- c(4, 4, 8, 13)

  fit <- coolbox_linear_regression(x, y, method = "closed_form")
  preds <- predict(fit, x)

  expect_s3_class(fit, "coolbox_linear_regression")
  expect_length(preds, nrow(x))
  expect_equal(unname(as.numeric(preds)), y, tolerance = 1e-6)
})
