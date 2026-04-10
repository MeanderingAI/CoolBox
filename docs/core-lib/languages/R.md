**R Binding Guide**

- **Purpose:** Public overview of the R package layouts currently present in the repository.
- **Location:** `_libraries/r_bindings`

## Surface

- Documentation namespaces: `coolbox::r::gui` and `coolbox::r::core`.
- The repository currently contains two R-facing package layouts:
  - `coolboxgui` in `_libraries/r_bindings`
  - `coolboxr` in `_libraries/r_bindings/coolboxr`
- The actual R package names remain `coolboxgui` and `coolboxr`.

## Install

For `coolboxr`:

```bash
Rscript -e "install.packages('Rcpp', repos = 'https://cloud.r-project.org')"
R CMD INSTALL _libraries/r_bindings/coolboxr
```

For `coolboxgui`:

```r
install.packages("Rcpp")
library(Rcpp)
setwd("_libraries/r_bindings")
Rcpp::compileAttributes()
```

Then use standard `R CMD build` and `R CMD INSTALL` commands.

## Build

For `coolboxr` development:

```bash
Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"
R CMD INSTALL _libraries/r_bindings/coolboxr
Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"
```

## Notes

- `coolboxr` is the broader analytical package and includes ML and time-series wrappers.
- `coolboxgui` is focused on GUI primitives.
- The unified docs workflow currently expects the packaged R documentation flow under `coolboxr`.

## Packaging

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `r-extension-*` artifacts.