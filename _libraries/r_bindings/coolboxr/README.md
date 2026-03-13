# coolboxr

`coolboxr` is a small R package scaffold for exposing selected CoolBox C++
components to R.

## Included binding

- `coolbox_linear_regression()` wraps the existing CoolBox linear regression
  implementation.

## Local development

From the repository root:

1. Generate roxygen docs:
   - `Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"`
2. Install the package:
   - `R CMD INSTALL _libraries/r_bindings/coolboxr`
3. Build the pkgdown site:
   - `Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"`

The generated site is written to `_libraries/r_bindings/coolboxr/docs`.
