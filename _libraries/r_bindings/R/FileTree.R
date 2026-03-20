#' FileTree GUI Primitive
#' @export
FileTree <- function(rootName) {
  structure(list(handle = .Call(`_coolboxgui_FileTree_create`, as.character(rootName))), class = "FileTree")
}
#' @export
FileTree_free <- function(filetree) {
  .Call(`_coolboxgui_FileTree_free`, filetree$handle)
}
