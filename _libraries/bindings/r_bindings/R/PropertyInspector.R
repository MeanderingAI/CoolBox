#' PropertyInspector GUI Primitive
#' @export
PropertyInspector <- function(keys, values) {
  structure(list(handle = .Call(`_coolboxgui_PropertyInspector_create`, as.character(keys), as.character(values))), class = "PropertyInspector")
}
#' @export
PropertyInspector_free <- function(inspector) {
  .Call(`_coolboxgui_PropertyInspector_free`, inspector$handle)
}
