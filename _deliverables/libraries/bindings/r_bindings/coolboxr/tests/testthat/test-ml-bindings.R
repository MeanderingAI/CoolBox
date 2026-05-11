test_that("decision tree and svm bindings work", {
  x_dt <- matrix(c(0L, 0L, 0L, 1L, 1L, 0L, 1L, 1L), ncol = 2, byrow = TRUE)
  y_dt <- c(0L, 1L, 1L, 0L)
  dt <- coolbox_decision_tree(x_dt, y_dt, max_depth = 3L)

  expect_length(predict(dt, matrix(c(0L, 1L), nrow = 1)), 1L)

  x_svm <- matrix(c(1, 2, 2, 3, 6, 5, 7, 7), ncol = 2, byrow = TRUE)
  y_svm <- c(-1, -1, 1, 1)
  svm <- coolbox_svm(x_svm, y_svm, kernel = "linear")

  expect_length(predict(svm, matrix(c(2, 2, 6, 6), ncol = 2, byrow = TRUE)), 2L)
})

test_that("bayesian network and hmm bindings work", {
  bn <- coolbox_bayesian_network()
  coolbox_bn_add_node(bn, "Weather", c("Sunny", "Rainy"))
  coolbox_bn_add_node(bn, "Sprinkler", c("On", "Off"))
  coolbox_bn_add_edge(bn, "Weather", "Sprinkler")
  coolbox_bn_set_cpt(bn, "Weather", matrix(c(0.7, 0.3), nrow = 1))
  coolbox_bn_set_cpt(bn, "Sprinkler", matrix(c(0.2, 0.8, 0.01, 0.99), nrow = 2, byrow = TRUE))

  posterior <- coolbox_bn_query(bn, "Sprinkler", list(Weather = "Sunny"))
  expect_equal(length(posterior), 2L)
  expect_true(all(posterior >= 0))

  hmm <- coolbox_hmm(
    2L,
    3L,
    c(0.6, 0.4),
    matrix(c(0.7, 0.3, 0.4, 0.6), nrow = 2, byrow = TRUE),
    matrix(c(0.5, 0.4, 0.1, 0.1, 0.3, 0.6), nrow = 2, byrow = TRUE)
  )

  expect_length(coolbox_hmm_viterbi(hmm, c(1L, 2L, 3L)), 3L)
  expect_true(is.finite(coolbox_hmm_log_likelihood(hmm, c(1L, 2L, 3L))))
})

test_that("dimensionality reduction bindings work", {
  set.seed(42)
  x <- matrix(rnorm(30), ncol = 3)

  pca <- coolbox_pca(x, n_components = 2L)
  expect_equal(ncol(predict(pca, x)), 2L)

  knn <- coolbox_knn(x, k = 2L)
  neighbors <- coolbox_kneighbors(knn)
  expect_true(all(c("indices", "distances") %in% names(neighbors)))

  umap <- coolbox_umap(x, n_components = 2L, n_neighbors = 3L, n_epochs = 10L)
  expect_equal(ncol(coolbox_umap_embedding(umap)), 2L)
})
