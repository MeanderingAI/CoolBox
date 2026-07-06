# R Extensions

## Location

- `_libraries/r_bindings`

## Surface

- Documentation namespaces: `coolbox::r::gui` and `coolbox::r::core`
- The repository currently contains two R-facing package layouts:
  - `coolboxgui` in `_libraries/r_bindings`
  - `coolboxr` in `_libraries/r_bindings/coolboxr`
- The actual R package names remain `coolboxgui` and `coolboxr`

## Installation

For the broader `coolboxr` package:

```bash
Rscript -e "install.packages('Rcpp', repos = 'https://cloud.r-project.org')"
R CMD INSTALL _libraries/r_bindings/coolboxr
```

For the GUI-focused `coolboxgui` package:

```r
install.packages("Rcpp")
library(Rcpp)
setwd("_libraries/r_bindings")
Rcpp::compileAttributes()
# Then run:
# R CMD build .
# R CMD INSTALL coolboxgui_0.1.0.tar.gz
```

## Setup And Build

For `coolboxr` local development:

```bash
Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"
R CMD INSTALL _libraries/r_bindings/coolboxr
Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"
```

For `coolboxgui` local setup, generate attributes and then use standard R package build commands.

## Setup Notes

- `coolboxr` is the broader analytical package and includes ML and time-series wrappers.
- `coolboxgui` is focused on GUI primitives such as `Toolbar`, `DockPanel`, `LayerList`, `PropertyInspector`, `FileTree`, `RadioSelector`, and `CheckboxGroup`.
- The unified docs workflow currently expects the packaged R documentation flow under `coolboxr`.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `r-extension-*` artifacts.