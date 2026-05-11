# coolboxr

`coolboxr` exposes native CoolBox C++ algorithms to R.

## Installation

From the repository root:

1. Install the runtime dependency:
    - `Rscript -e "install.packages('Rcpp', repos = 'https://cloud.r-project.org')"`
2. Install the package:
    - `R CMD INSTALL _libraries/r_bindings/coolboxr`

## Included bindings

- `coolbox_decision_tree()` with Gini and entropy splitting plus max-depth
   control for integer-valued features.
- `coolbox_svm()` with linear, RBF, polynomial, and sigmoid kernels.
- `coolbox_bayesian_network()` plus helpers for DAG construction, CPT setup,
   exact inference, and joint-probability queries.
- `coolbox_hmm()` with parameter management, Baum-Welch training,
   forward log-likelihood, and Viterbi decoding.
- `coolbox_linear_regression()` wraps the existing CoolBox linear regression
   implementation.
- `coolbox_multi_arm_bandit()` for arm simulations and pull statistics.
- `coolbox_marked_point_process()` for marked temporal events, Hawkes-style
   excitation, intensity prediction, and sequence generation.
- `coolbox_pcim()` for non-stationary piecewise conditional intensity models,
   interval creation, regime-aware intensity estimation, and information
   criteria.
- `coolbox_latent_sentiment_analysis()` for matrix-factorization-based
   sentiment modeling.
- `coolbox_svd()`, `coolbox_pca()`, `coolbox_knn()`, and `coolbox_umap()` for
   dimensionality reduction and nearest-neighbor workflows.
- `coolbox_time_series_summary()` exposes descriptive statistics and
   autocorrelation helpers from the CoolBox time-series library.
- `coolbox_time_series_transform()` exposes common native transforms such as
   normalization, differencing, moving averages, smoothing, and resampling.
- `coolbox_time_series_forecast()` wraps native moving-average,
   exponential-smoothing, and autoregressive forecasters.
- `coolbox_time_series_outliers()` exposes native z-score and IQR outlier
   detection helpers.

## Local development

From the repository root:

1. Generate roxygen docs:
   - `Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"`
2. Install the package:
   - `R CMD INSTALL _libraries/r_bindings/coolboxr`
3. Build the pkgdown site:
   - `Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"`

The generated site is written to `_libraries/r_bindings/coolboxr/docs`.
