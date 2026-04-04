# coolboxr

`coolboxr` exposes native CoolBox C++ algorithms to R.

## Installation

From the repository root:

1.  Install the runtime dependency:
    - `Rscript -e "install.packages('Rcpp', repos = 'https://cloud.r-project.org')"`
2.  Install the package:
    - `R CMD INSTALL _libraries/r_bindings/coolboxr`

## Included bindings

- [`coolbox_decision_tree()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_decision_tree.md)
  with Gini and entropy splitting plus max-depth control for
  integer-valued features.
- [`coolbox_svm()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_svm.md)
  with linear, RBF, polynomial, and sigmoid kernels.
- [`coolbox_bayesian_network()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_bayesian_network.md)
  plus helpers for DAG construction, CPT setup, exact inference, and
  joint-probability queries.
- [`coolbox_hmm()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_hmm.md)
  with parameter management, Baum-Welch training, forward
  log-likelihood, and Viterbi decoding.
- [`coolbox_linear_regression()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_linear_regression.md)
  wraps the existing CoolBox linear regression implementation.
- [`coolbox_multi_arm_bandit()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_multi_arm_bandit.md)
  for arm simulations and pull statistics.
- [`coolbox_marked_point_process()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_marked_point_process.md)
  for marked temporal events, Hawkes-style excitation, intensity
  prediction, and sequence generation.
- [`coolbox_pcim()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_pcim.md)
  for non-stationary piecewise conditional intensity models, interval
  creation, regime-aware intensity estimation, and information criteria.
- [`coolbox_latent_sentiment_analysis()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_latent_sentiment_analysis.md)
  for matrix-factorization-based sentiment modeling.
- [`coolbox_svd()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_svd.md),
  [`coolbox_pca()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_pca.md),
  [`coolbox_knn()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_knn.md),
  and
  [`coolbox_umap()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_umap.md)
  for dimensionality reduction and nearest-neighbor workflows.
- [`coolbox_time_series_summary()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_time_series_summary.md)
  exposes descriptive statistics and autocorrelation helpers from the
  CoolBox time-series library.
- [`coolbox_time_series_transform()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_time_series_transform.md)
  exposes common native transforms such as normalization, differencing,
  moving averages, smoothing, and resampling.
- [`coolbox_time_series_forecast()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_time_series_forecast.md)
  wraps native moving-average, exponential-smoothing, and autoregressive
  forecasters.
- [`coolbox_time_series_outliers()`](https://mehranghamaty.github.io/CoolBox/reference/coolbox_time_series_outliers.md)
  exposes native z-score and IQR outlier detection helpers.

## Local development

From the repository root:

1.  Generate roxygen docs:
    - `Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"`
2.  Install the package:
    - `R CMD INSTALL _libraries/r_bindings/coolboxr`
3.  Build the pkgdown site:
    - `Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"`

The generated site is written to `_libraries/r_bindings/coolboxr/docs`.
