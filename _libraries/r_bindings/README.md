# coolboxgui R Package

This package provides R bindings for CoolBox GUI primitives:
- Toolbar
- DockPanel
- LayerList
- PropertyInspector
- FileTree
- RadioSelector
- CheckboxGroup

## Usage Example

```r
library(coolboxgui)

# Create a toolbar
tb <- Toolbar(c("Open", "Save", "Exit"))
Toolbar_free(tb)

# Create a dock panel
dp <- DockPanel("My Panel", floating = TRUE)
DockPanel_free(dp)

# Create a layer list
ll <- LayerList(c("Layer1", "Layer2"), selected = 2)
LayerList_free(ll)

# Create a property inspector
pi <- PropertyInspector(c("Width", "Height"), c("100", "200"))
PropertyInspector_free(pi)

# Create a file tree
ft <- FileTree("/")
FileTree_free(ft)

# Create a radio selector
rs <- RadioSelector(c("A", "B", "C"), selected = 1)
RadioSelector_free(rs)

# Create a checkbox group
cbg <- CheckboxGroup(c("Opt1", "Opt2"), c(TRUE, FALSE))
CheckboxGroup_free(cbg)
```

## Build & Install

From the R console:

```r
install.packages("Rcpp")
library(Rcpp)
setwd("_libraries/r_bindings")
Rcpp::compileAttributes()
# Then build and install as usual:
# R CMD build .
# R CMD INSTALL coolboxgui_0.1.0.tar.gz
```
