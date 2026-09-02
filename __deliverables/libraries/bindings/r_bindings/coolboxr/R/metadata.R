#' Create a default CoolBox metadata client
#'
#' @return An object of class `coolbox_client`.
#' @export
coolbox_create_default <- function() {
  structure(list(endpoint = "local://coolbox"), class = "coolbox_client")
}

#' Create a CoolBox metadata client for an endpoint
#'
#' @param endpoint Endpoint string.
#'
#' @return An object of class `coolbox_client`.
#' @export
coolbox_for_endpoint <- function(endpoint) {
  endpoint <- as.character(endpoint[[1]])
  if (!nzchar(endpoint)) {
    endpoint <- "local://coolbox"
  }
  structure(list(endpoint = endpoint), class = "coolbox_client")
}

#' Get the client endpoint
#'
#' @param client A `coolbox_client` object.
#'
#' @return A length-one character vector.
#' @export
coolbox_endpoint <- function(client) {
  stopifnot(inherits(client, "coolbox_client"))
  client$endpoint
}

#' Get the CoolBox metadata version
#'
#' @param client A `coolbox_client` object.
#'
#' @return A length-one character vector.
#' @export
coolbox_version <- function(client) {
  stopifnot(inherits(client, "coolbox_client"))
  .Call(`_coolboxr_metadata_version`)
}

#' Get the CoolBox metadata description
#'
#' @param client A `coolbox_client` object.
#'
#' @return A length-one character vector.
#' @export
coolbox_describe <- function(client) {
  stopifnot(inherits(client, "coolbox_client"))
  .Call(`_coolboxr_metadata_describe`)
}

#' Get the advertised CoolBox capabilities
#'
#' @param client A `coolbox_client` object.
#'
#' @return A character vector.
#' @export
coolbox_capabilities <- function(client) {
  stopifnot(inherits(client, "coolbox_client"))
  count <- .Call(`_coolboxr_metadata_capability_count`)
  vapply(seq_len(count) - 1L, function(index) {
    .Call(`_coolboxr_metadata_capability_at`, as.integer(index))
  }, character(1))
}

#' Get a capability at a zero-based index
#'
#' @param client A `coolbox_client` object.
#' @param index Zero-based capability index.
#'
#' @return A length-one character vector or `""` when out of range.
#' @export
coolbox_capability_at <- function(client, index) {
  stopifnot(inherits(client, "coolbox_client"))
  .Call(`_coolboxr_metadata_capability_at`, as.integer(index))
}

#' Check whether the metadata bindings are ready
#'
#' @param client A `coolbox_client` object.
#'
#' @return A logical scalar.
#' @export
coolbox_is_ready <- function(client) {
  stopifnot(inherits(client, "coolbox_client"))
  .Call(`_coolboxr_metadata_is_ready`)
}