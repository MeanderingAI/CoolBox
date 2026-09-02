#' Generate UUID version 1
#' @export
coolbox_uuid_v1 <- function() {
  .Call(`_coolboxr_uuid_v1`)
}

#' Generate UUID version 2 (DCE security style)
#' @param local_identifier Integer local identifier
#' @param local_domain Integer local domain value
#' @export
coolbox_uuid_v2 <- function(local_identifier = 0L, local_domain = 0L) {
  .Call(`_coolboxr_uuid_v2`, as.integer(local_identifier), as.integer(local_domain))
}

#' Generate UUID version 3 (name-based MD5)
#' @param namespace_uuid Namespace UUID string
#' @param name Name input string
#' @export
coolbox_uuid_v3 <- function(namespace_uuid, name) {
  .Call(`_coolboxr_uuid_v3`, as.character(namespace_uuid), as.character(name))
}

#' Generate UUID version 4 (random)
#' @export
coolbox_uuid_v4 <- function() {
  .Call(`_coolboxr_uuid_v4`)
}

#' Generate UUID version 5 (name-based SHA-1)
#' @param namespace_uuid Namespace UUID string
#' @param name Name input string
#' @export
coolbox_uuid_v5 <- function(namespace_uuid, name) {
  .Call(`_coolboxr_uuid_v5`, as.character(namespace_uuid), as.character(name))
}

#' Generate UUID version 6 (sortable timestamp)
#' @export
coolbox_uuid_v6 <- function() {
  .Call(`_coolboxr_uuid_v6`)
}

#' Generate UUID version 8 (custom entropy)
#' @param entropy_hex Hex string consumed as custom entropy
#' @export
coolbox_uuid_v8 <- function(entropy_hex = "") {
  .Call(`_coolboxr_uuid_v8`, as.character(entropy_hex))
}

#' Generate GUID string
#' @export
coolbox_guid <- function() {
  .Call(`_coolboxr_guid`)
}
