# Eigen:: References Report (as of 2026-04-20)

This report lists all current references to `Eigen::` in the codebase. These should be migrated to `mytrix::` where possible.

---

## Matches

- _libraries/python_bindings/include/tracker/extended_kalman_filter.h
  - Line 64: void setProcessModel(const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f,
  - Line 65: const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F);
  - Line 67: void setMeasurementModel(const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h,
  - Line 68: const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H);
  - Line 72: void update(const Eigen::VectorXd& z) override;
  - Line 74: const Eigen::VectorXd& state() const override;
  - Line 75: const Eigen::MatrixXd& covariance() const override;
  - Line 78: std::function<Eigen::VectorXd(const Eigen::VectorXd&)> f_;
  - Line 79: std::function<Eigen::MatrixXd(const Eigen::VectorXd&)> F_;
  - Line 80: std::function<Eigen::VectorXd(const Eigen::VectorXd&)> h_;
  - Line 81: std::function<Eigen::MatrixXd(const Eigen::VectorXd&)> H_;
  - Line 82: Eigen::VectorXd x_;
  - Line 83: Eigen::MatrixXd P_;
  - Line 84: Eigen::MatrixXd Q_;
  - Line 85: Eigen::MatrixXd R_;

- _libraries/python_bindings/include/latent_sentiment_analysis/latent_sentiment_analysis.h
  - Line 42: * @param document_term_matrix The input matrix. It must be convertible to Eigen::MatrixXd.
  - Line 44: void train(const Eigen::MatrixXd& document_term_matrix);
  - Line 58: const Eigen::MatrixXd& get_document_features() const { return U; }
  - Line 64: const Eigen::MatrixXd& get_term_features() const { return V; }
  - Line 76: Eigen::MatrixXd U;
  - Line 79: Eigen::MatrixXd V;

- README.md
  - Line 206: Eigen::MatrixXd X = /* your data (n_samples x n_features) */;
  - Line 209: Eigen::MatrixXd X_pca = pca.transform(X);
  - Line 219: Eigen::MatrixXd X_umap = umap.fit_transform(X);

- __GENERATED_CONTENT/read_mes/python_bindings_README.md
  - Line 598: Eigen::MatrixXd Xc = X.rowwise() - X.colwise().mean().transpose();
  - Line 601: Eigen::BDCSVD<Eigen::MatrixXd> svd(Xc, Eigen::ComputeThinU | Eigen::ComputeThinV);
  - Line 602: Eigen::MatrixXd U = svd.matrixU();
  - Line 603: Eigen::VectorXd S = svd.singularValues();
  - Line 604: Eigen::MatrixXd V = svd.matrixV();

---

> Note: Many other Eigen:: references exist in third-party, build, or test files (e.g., pybind11, Eigen's own test suite). Only project source and documentation files are listed above.
