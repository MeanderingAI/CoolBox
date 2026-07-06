#' DockPanel GUI Primitive
#' @export
DockPanel <- function(title, floating = FALSE) {
  structure(list(handle = .Call(`_coolboxgui_DockPanel_create`, as.character(title), as.logical(floating))), class = "DockPanel")
}
#' @export
DockPanel_free <- function(dockpanel) {
  .Call(`_coolboxgui_DockPanel_free`, dockpanel$handle)
}
