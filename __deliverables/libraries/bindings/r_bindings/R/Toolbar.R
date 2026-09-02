#' Toolbar GUI Primitive
#' @export
Toolbar <- function(actions) {
  structure(list(handle = .Call(`_coolboxgui_Toolbar_create`, as.character(actions))), class = "Toolbar")
}
#' @export
Toolbar_free <- function(toolbar) {
  .Call(`_coolboxgui_Toolbar_free`, toolbar$handle)
}
