#' LayerList GUI Primitive
#' @export
LayerList <- function(layers, selected = 1L) {
  structure(list(handle = .Call(`_coolboxgui_LayerList_create`, as.character(layers), as.integer(selected))), class = "LayerList")
}
#' @export
LayerList_free <- function(layerlist) {
  .Call(`_coolboxgui_LayerList_free`, layerlist$handle)
}
