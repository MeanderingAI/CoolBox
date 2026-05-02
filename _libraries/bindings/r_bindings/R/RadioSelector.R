#' RadioSelector GUI Primitive
#' @export
RadioSelector <- function(options, selected = 1L) {
  structure(list(handle = .Call(`_coolboxgui_RadioSelector_create`, as.character(options), as.integer(selected))), class = "RadioSelector")
}
#' @export
RadioSelector_free <- function(radio) {
  .Call(`_coolboxgui_RadioSelector_free`, radio$handle)
}
