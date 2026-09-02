#' CheckboxGroup GUI Primitive
#' @export
CheckboxGroup <- function(options, checked) {
  structure(list(handle = .Call(`_coolboxgui_CheckboxGroup_create`, as.character(options), as.logical(checked))), class = "CheckboxGroup")
}
#' @export
CheckboxGroup_free <- function(checkbox) {
  .Call(`_coolboxgui_CheckboxGroup_free`, checkbox$handle)
}
