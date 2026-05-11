as_numeric_matrix <- function(x, allow_integer = FALSE) {
  x <- as.matrix(x)
  if (!is.matrix(x)) {
    stop("Input must be coercible to a matrix.", call. = FALSE)
  }
  if (!(is.numeric(x) || (allow_integer && is.integer(x)))) {
    stop("Matrix must be numeric.", call. = FALSE)
  }
  x
}

as_integer_matrix <- function(x) {
  x <- as.matrix(x)
  storage.mode(x) <- "integer"
  x
}

as_integer_vector0 <- function(x) {
  x <- as.integer(x)
  if (any(is.na(x))) {
    stop("Indices must be finite integers.", call. = FALSE)
  }
  x - 1L
}

wrap_external_pointer <- function(ptr, class_name, metadata = list()) {
  structure(c(list(ptr = ptr), metadata), class = class_name)
}

#' Fit a CoolBox decision tree classifier
#' @export
coolbox_decision_tree <- function(x, y, criterion = c("gini", "entropy"), max_depth = 5L) {
  criterion <- match.arg(criterion)
  x <- as_integer_matrix(x)
  y <- as.integer(y)
  ptr <- .Call(`_coolboxr_fit_decision_tree`, x, y, criterion, as.integer(max_depth))
  wrap_external_pointer(ptr, "coolbox_decision_tree", list(criterion = criterion, max_depth = as.integer(max_depth)))
}

#' @export
predict.coolbox_decision_tree <- function(object, newdata, ...) {
  .Call(`_coolboxr_predict_decision_tree`, object$ptr, as_integer_matrix(newdata))
}

#' Fit a CoolBox support vector machine
#' @export
coolbox_svm <- function(x, y, kernel = c("linear", "rbf", "polynomial", "sigmoid"), gamma = 1, coef0 = 0, degree = 3L) {
  kernel <- match.arg(kernel)
  x <- as_numeric_matrix(x)
  storage.mode(x) <- "double"
  y <- as.numeric(y)
  ptr <- .Call(`_coolboxr_fit_svm`, x, y, kernel, as.numeric(gamma), as.numeric(coef0), as.integer(degree))
  wrap_external_pointer(ptr, "coolbox_svm", list(kernel = kernel, gamma = gamma, coef0 = coef0, degree = as.integer(degree)))
}

#' @export
predict.coolbox_svm <- function(object, newdata, ...) {
  newdata <- as_numeric_matrix(newdata)
  storage.mode(newdata) <- "double"
  .Call(`_coolboxr_predict_svm`, object$ptr, newdata)
}

#' Create a Bayesian network
#' @export
coolbox_bayesian_network <- function() {
  wrap_external_pointer(.Call(`_coolboxr_create_bayesian_network`), "coolbox_bayesian_network")
}

coolbox_bn_node_id <- function(object, node) {
  if (is.character(node)) {
    .Call(`_coolboxr_bn_get_node_id`, object$ptr, as.character(node[[1L]])) + 1L
  } else {
    as.integer(node[[1L]])
  }
}

coolbox_bn_state_id <- function(state) {
  if (is.character(state)) {
    stop("Character state labels are only supported inside named evidence or assignment lists.", call. = FALSE)
  }
  as.integer(state[[1L]])
}

resolve_bn_state <- function(object, node, state) {
  node_info <- coolbox_bn_nodes(object)[[coolbox_bn_node_id(object, node)]]
  if (is.character(state)) {
    idx <- match(state[[1L]], node_info$states)
    if (is.na(idx)) {
      stop("Unknown state label for node.", call. = FALSE)
    }
    idx
  } else {
    as.integer(state[[1L]])
  }
}

bn_pairs_from_mapping <- function(object, mapping) {
  if (length(mapping) == 0L) {
    return(list(nodes = integer(), states = integer()))
  }

  if (is.null(names(mapping)) || any(names(mapping) == "")) {
    stop("Bayesian network mappings must be named by node.", call. = FALSE)
  }

  nodes <- integer(length(mapping))
  states <- integer(length(mapping))
  for (i in seq_along(mapping)) {
    nodes[[i]] <- coolbox_bn_node_id(object, names(mapping)[[i]])
    states[[i]] <- resolve_bn_state(object, names(mapping)[[i]], mapping[[i]])
  }

  list(nodes = nodes - 1L, states = states - 1L)
}

#' @export
coolbox_bn_add_node <- function(object, name, states) {
  id <- .Call(`_coolboxr_bn_add_node`, object$ptr, as.character(name), as.character(states)) + 1L
  invisible(id)
}

#' @export
coolbox_bn_add_edge <- function(object, parent, child) {
  .Call(`_coolboxr_bn_add_edge`, object$ptr, coolbox_bn_node_id(object, parent) - 1L, coolbox_bn_node_id(object, child) - 1L)
  invisible(object)
}

#' @export
coolbox_bn_set_cpt <- function(object, node, cpt) {
  cpt <- as.matrix(cpt)
  storage.mode(cpt) <- "double"
  .Call(`_coolboxr_bn_set_cpt`, object$ptr, coolbox_bn_node_id(object, node) - 1L, cpt)
  invisible(object)
}

#' @export
coolbox_bn_query <- function(object, query, evidence = list()) {
  pairs <- bn_pairs_from_mapping(object, evidence)
  .Call(`_coolboxr_bn_query`, object$ptr, coolbox_bn_node_id(object, query) - 1L, as.integer(pairs$nodes), as.integer(pairs$states))
}

#' @export
coolbox_bn_infer <- function(object, query, state, evidence = list()) {
  distribution <- coolbox_bn_query(object, query, evidence)
  distribution[[resolve_bn_state(object, query, state)]]
}

#' @export
coolbox_bn_joint_probability <- function(object, assignment) {
  if (is.null(names(assignment)) || any(names(assignment) == "")) {
    stop("Assignment must be a named vector or list.", call. = FALSE)
  }
  nodes <- coolbox_bn_nodes(object)
  full_assignment <- integer(length(nodes))
  for (i in seq_along(nodes)) {
    node_name <- nodes[[i]]$name
    if (!node_name %in% names(assignment)) {
      stop("Assignment must include all nodes.", call. = FALSE)
    }
    full_assignment[[i]] <- resolve_bn_state(object, node_name, assignment[[node_name]])
  }
  .Call(`_coolboxr_bn_joint_probability`, object$ptr, as.integer(full_assignment - 1L))
}

#' @export
coolbox_bn_nodes <- function(object) {
  nodes <- .Call(`_coolboxr_bn_nodes`, object$ptr)
  lapply(nodes, function(node) {
    node$parents <- node$parents + 1L
    node$children <- node$children + 1L
    node
  })
}

#' Create a Hidden Markov Model
#' @export
coolbox_hmm <- function(states, observations, initial_probabilities = NULL, transition_matrix = NULL, emission_matrix = NULL) {
  ptr <- .Call(`_coolboxr_create_hmm`, as.integer(states), as.integer(observations))
  object <- wrap_external_pointer(ptr, "coolbox_hmm", list(states = as.integer(states), observations = as.integer(observations)))
  if (!is.null(initial_probabilities) || !is.null(transition_matrix) || !is.null(emission_matrix)) {
    coolbox_hmm_set_parameters(object, initial_probabilities, transition_matrix, emission_matrix)
  }
  object
}

#' @export
coolbox_hmm_set_parameters <- function(object, initial_probabilities, transition_matrix, emission_matrix) {
  .Call(
    `_coolboxr_hmm_set_parameters`,
    object$ptr,
    as.numeric(initial_probabilities),
    as.matrix(transition_matrix),
    as.matrix(emission_matrix)
  )
  invisible(object)
}

#' @export
coolbox_hmm_parameters <- function(object) {
  .Call(`_coolboxr_hmm_get_parameters`, object$ptr)
}

#' @export
coolbox_hmm_train <- function(object, sequences, max_iterations = 100L, tolerance = 1e-6, smoothing_factor = 0, seed = 0L) {
  sequences <- lapply(sequences, as_integer_vector0)
  .Call(`_coolboxr_hmm_train`, object$ptr, sequences, as.integer(max_iterations), as.numeric(tolerance), as.numeric(smoothing_factor), as.integer(seed))
  invisible(object)
}

#' @export
coolbox_hmm_viterbi <- function(object, observations) {
  .Call(`_coolboxr_hmm_viterbi`, object$ptr, as_integer_vector0(observations)) + 1L
}

#' @export
coolbox_hmm_log_likelihood <- function(object, observations) {
  .Call(`_coolboxr_hmm_log_likelihood`, object$ptr, as_integer_vector0(observations))
}

#' Run a multi-arm bandit simulation
#' @export
coolbox_multi_arm_bandit <- function(true_probs, strategy = c("epsilon_greedy", "ucb", "thompson_sampling", "decaying_epsilon"), steps = 1000L, epsilon = 0.1, c = 2, decay_rate = 0.99, seed = 0L) {
  strategy <- match.arg(strategy)
  .Call(`_coolboxr_run_bandit_simulation`, as.numeric(true_probs), strategy, as.integer(steps), as.numeric(epsilon), as.numeric(c), as.numeric(decay_rate), as.integer(seed))
}

#' Fit a marked point process
#' @export
coolbox_marked_point_process <- function(event_times = NULL, event_marks = NULL, num_marks = 2L, learning_rate = 0.01, max_iterations = 1000L) {
  ptr <- .Call(`_coolboxr_create_marked_point_process`, as.integer(num_marks), as.numeric(learning_rate), as.integer(max_iterations))
  object <- wrap_external_pointer(ptr, "coolbox_marked_point_process", list(num_marks = as.integer(num_marks)))
  if (!is.null(event_times) && !is.null(event_marks)) {
    coolbox_mpp_fit(object, event_times, event_marks)
  }
  object
}

normalize_mark_sequences <- function(event_marks) {
  lapply(event_marks, as_integer_vector0)
}

#' @export
coolbox_mpp_fit <- function(object, event_times, event_marks) {
  .Call(`_coolboxr_mpp_fit`, object$ptr, lapply(event_times, as.numeric), normalize_mark_sequences(event_marks))
  invisible(object)
}

#' @export
coolbox_mpp_predict_intensity <- function(object, time, history_times, history_marks) {
  .Call(`_coolboxr_mpp_predict_intensity`, object$ptr, as.numeric(time), as.numeric(history_times), as_integer_vector0(history_marks))
}

#' @export
coolbox_mpp_generate_sequence <- function(object, time_horizon, max_events = 1000L) {
  out <- .Call(`_coolboxr_mpp_generate_sequence`, object$ptr, as.numeric(time_horizon), as.integer(max_events))
  out$event_marks <- out$event_marks + 1L
  out
}

#' @export
coolbox_mpp_log_likelihood <- function(object, event_times, event_marks) {
  .Call(`_coolboxr_mpp_log_likelihood`, object$ptr, lapply(event_times, as.numeric), normalize_mark_sequences(event_marks))
}

#' @export
coolbox_mpp_parameters <- function(object) {
  .Call(`_coolboxr_mpp_parameters`, object$ptr)
}

#' Create a piecewise conditional intensity model
#' @export
coolbox_pcim <- function(num_intervals = 10L, learning_rate = 0.01, max_iterations = 1000L) {
  wrap_external_pointer(.Call(`_coolboxr_create_pcim`, as.integer(num_intervals), as.numeric(learning_rate), as.integer(max_iterations)), "coolbox_pcim")
}

#' @export
coolbox_pcim_create_uniform_intervals <- function(object, time_min, time_max, intensity_type = c("constant", "linear", "exponential", "hawkes", "cox")) {
  intensity_type <- match.arg(intensity_type)
  .Call(`_coolboxr_pcim_create_uniform_intervals`, object$ptr, as.numeric(time_min), as.numeric(time_max), intensity_type)
  invisible(object)
}

#' @export
coolbox_pcim_create_adaptive_intervals <- function(object, event_times, intensity_type = c("constant", "linear", "exponential", "hawkes", "cox")) {
  intensity_type <- match.arg(intensity_type)
  .Call(`_coolboxr_pcim_create_adaptive_intervals`, object$ptr, sort(as.numeric(event_times)), intensity_type)
  invisible(object)
}

#' @export
coolbox_pcim_fit <- function(object, event_times) {
  .Call(`_coolboxr_pcim_fit`, object$ptr, lapply(event_times, as.numeric))
  invisible(object)
}

#' @export
coolbox_pcim_fit_with_covariates <- function(object, event_times, covariates) {
  .Call(`_coolboxr_pcim_fit_with_covariates`, object$ptr, lapply(event_times, as.numeric), lapply(covariates, as.matrix))
  invisible(object)
}

#' @export
coolbox_pcim_predict_intensity <- function(object, time, history_times) {
  .Call(`_coolboxr_pcim_predict_intensity`, object$ptr, as.numeric(time), as.numeric(history_times))
}

#' @export
coolbox_pcim_predict_intensity_with_covariates <- function(object, time, history_times, covariates) {
  .Call(`_coolboxr_pcim_predict_intensity_with_covariates`, object$ptr, as.numeric(time), as.numeric(history_times), as.numeric(covariates))
}

#' @export
coolbox_pcim_generate_sequence <- function(object, time_horizon, max_events = 1000L) {
  .Call(`_coolboxr_pcim_generate_sequence`, object$ptr, as.numeric(time_horizon), as.integer(max_events))
}

#' @export
coolbox_pcim_log_likelihood <- function(object, event_times) {
  .Call(`_coolboxr_pcim_log_likelihood`, object$ptr, lapply(event_times, as.numeric))
}

#' @export
coolbox_pcim_information_criteria <- function(object, event_times) {
  .Call(`_coolboxr_pcim_information_criteria`, object$ptr, lapply(event_times, as.numeric))
}

#' @export
coolbox_pcim_intervals <- function(object) {
  .Call(`_coolboxr_pcim_intervals`, object$ptr)
}

#' Fit latent sentiment analysis
#' @export
coolbox_latent_sentiment_analysis <- function(document_term_matrix, latent_features = 2L, learning_rate = 0.01, lambda = 0.1, max_iterations = 1000L) {
  ptr <- .Call(
    `_coolboxr_fit_latent_sentiment_analysis`,
    as.matrix(document_term_matrix),
    as.integer(latent_features),
    as.numeric(learning_rate),
    as.numeric(lambda),
    as.integer(max_iterations)
  )
  wrap_external_pointer(ptr, "coolbox_latent_sentiment_analysis")
}

#' @export
coolbox_lsa_predict_score <- function(object, document_index, term_index) {
  .Call(`_coolboxr_lsa_predict_score`, object$ptr, as.integer(document_index) - 1L, as.integer(term_index) - 1L)
}

#' @export
coolbox_lsa_factors <- function(object) {
  .Call(`_coolboxr_lsa_factors`, object$ptr)
}

#' Compute singular value decomposition
#' @export
coolbox_svd <- function(x, full_matrices = FALSE) {
  ptr <- .Call(`_coolboxr_fit_svd`, as.matrix(x), isTRUE(full_matrices))
  wrap_external_pointer(ptr, "coolbox_svd")
}

#' @export
coolbox_svd_summary <- function(object) {
  .Call(`_coolboxr_svd_summary`, object$ptr)
}

#' @export
coolbox_svd_reconstruct <- function(object, num_components = 0L) {
  .Call(`_coolboxr_svd_reconstruct`, object$ptr, as.integer(num_components))
}

#' Fit principal component analysis
#' @export
coolbox_pca <- function(x, n_components = 0L, center = TRUE, scale = FALSE) {
  ptr <- .Call(`_coolboxr_fit_pca`, as.matrix(x), as.integer(n_components), isTRUE(center), isTRUE(scale))
  wrap_external_pointer(ptr, "coolbox_pca", list(n_components = as.integer(n_components), center = isTRUE(center), scale = isTRUE(scale)))
}

#' @export
predict.coolbox_pca <- function(object, newdata, ...) {
  .Call(`_coolboxr_pca_transform`, object$ptr, as.matrix(newdata))
}

#' @export
coolbox_pca_inverse_transform <- function(object, x) {
  .Call(`_coolboxr_pca_inverse_transform`, object$ptr, as.matrix(x))
}

#' @export
coolbox_pca_summary <- function(object) {
  .Call(`_coolboxr_pca_summary`, object$ptr)
}

#' Fit k-nearest neighbors search
#' @export
coolbox_knn <- function(x, k = 5L, metric = c("euclidean", "manhattan", "cosine")) {
  metric <- match.arg(metric)
  ptr <- .Call(`_coolboxr_fit_knn`, as.matrix(x), as.integer(k), metric)
  wrap_external_pointer(ptr, "coolbox_knn", list(k = as.integer(k), metric = metric))
}

#' @export
coolbox_kneighbors <- function(object, newdata = NULL) {
  if (is.null(newdata)) {
    .Call(`_coolboxr_knn_kneighbors`, object$ptr, NULL)
  } else {
    .Call(`_coolboxr_knn_kneighbors`, object$ptr, as.matrix(newdata))
  }
}

#' @export
coolbox_pairwise_distances <- function(object, x, y) {
  .Call(`_coolboxr_knn_pairwise_distances`, object$ptr, as.matrix(x), as.matrix(y))
}

#' Fit UMAP
#' @export
coolbox_umap <- function(x, n_components = 2L, n_neighbors = 15L, min_dist = 0.1, metric = c("euclidean", "manhattan", "cosine"), learning_rate = 1, n_epochs = 200L, random_state = 42L) {
  metric <- match.arg(metric)
  ptr <- .Call(
    `_coolboxr_fit_umap`,
    as.matrix(x),
    as.integer(n_components),
    as.integer(n_neighbors),
    as.numeric(min_dist),
    metric,
    as.numeric(learning_rate),
    as.integer(n_epochs),
    as.integer(random_state)
  )
  wrap_external_pointer(ptr, "coolbox_umap")
}

#' @export
predict.coolbox_umap <- function(object, newdata, ...) {
  .Call(`_coolboxr_umap_transform`, object$ptr, as.matrix(newdata))
}

#' @export
coolbox_umap_embedding <- function(object) {
  .Call(`_coolboxr_umap_embedding`, object$ptr)
}
