# Fit CoolBox linear regression

Train a small linear regression model backed by the native CoolBox C++
implementation.

## Usage

``` r
coolbox_linear_regression(
  x,
  y,
  method = c("closed_form", "gradient_descent"),
  iterations = 1000L,
  learning_rate = 0.01
)
```

## Arguments

- x:

  Numeric matrix or data frame of predictors.

- y:

  Numeric response vector.

- method:

  Fitting method. Either `"closed_form"` or `"gradient_descent"`.

- iterations:

  Number of gradient descent iterations. Ignored for the closed-form
  solver.

- learning_rate:

  Gradient descent learning rate.

## Value

An object of class `coolbox_linear_regression`.

## Examples

``` r
x <- matrix(c(1, 2, 2, 0, 3, 1, 4, 3), ncol = 2, byrow = TRUE)
y <- c(3, 2, 5, 8)
fit <- coolbox_linear_regression(x, y)
#> Error in coolbox_linear_regression(x, y): function 'Rcpp_precious_remove' not provided by package 'Rcpp'
predict(fit, x)
#> Error: object 'fit' not found
```
