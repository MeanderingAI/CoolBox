#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

extern SEXP _coolboxr_fit_linear_regression(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_predict_linear_regression(SEXP, SEXP);
extern SEXP _coolboxr_time_series_summary(SEXP, SEXP);
extern SEXP _coolboxr_transform_time_series(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_forecast_time_series(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_detect_time_series_outliers(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_fit_decision_tree(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_predict_decision_tree(SEXP, SEXP);
extern SEXP _coolboxr_fit_svm(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_predict_svm(SEXP, SEXP);
extern SEXP _coolboxr_create_bayesian_network(void);
extern SEXP _coolboxr_bn_add_node(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_bn_add_edge(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_bn_set_cpt(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_bn_get_node_id(SEXP, SEXP);
extern SEXP _coolboxr_bn_query(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_bn_joint_probability(SEXP, SEXP);
extern SEXP _coolboxr_bn_nodes(SEXP);
extern SEXP _coolboxr_create_hmm(SEXP, SEXP);
extern SEXP _coolboxr_hmm_set_parameters(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_hmm_get_parameters(SEXP);
extern SEXP _coolboxr_hmm_train(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_hmm_viterbi(SEXP, SEXP);
extern SEXP _coolboxr_hmm_log_likelihood(SEXP, SEXP);
extern SEXP _coolboxr_run_bandit_simulation(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_create_marked_point_process(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_mpp_fit(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_mpp_predict_intensity(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_mpp_generate_sequence(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_mpp_log_likelihood(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_mpp_parameters(SEXP);
extern SEXP _coolboxr_create_pcim(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_create_uniform_intervals(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_create_adaptive_intervals(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_fit(SEXP, SEXP);
extern SEXP _coolboxr_pcim_fit_with_covariates(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_predict_intensity(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_predict_intensity_with_covariates(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_generate_sequence(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pcim_log_likelihood(SEXP, SEXP);
extern SEXP _coolboxr_pcim_information_criteria(SEXP, SEXP);
extern SEXP _coolboxr_pcim_intervals(SEXP);
extern SEXP _coolboxr_fit_latent_sentiment_analysis(SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_lsa_predict_score(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_lsa_factors(SEXP);
extern SEXP _coolboxr_fit_svd(SEXP, SEXP);
extern SEXP _coolboxr_svd_summary(SEXP);
extern SEXP _coolboxr_svd_reconstruct(SEXP, SEXP);
extern SEXP _coolboxr_fit_pca(SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_pca_transform(SEXP, SEXP);
extern SEXP _coolboxr_pca_inverse_transform(SEXP, SEXP);
extern SEXP _coolboxr_pca_summary(SEXP);
extern SEXP _coolboxr_fit_knn(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_knn_kneighbors(SEXP, SEXP);
extern SEXP _coolboxr_knn_pairwise_distances(SEXP, SEXP, SEXP);
extern SEXP _coolboxr_fit_umap(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP _coolboxr_umap_transform(SEXP, SEXP);
extern SEXP _coolboxr_umap_embedding(SEXP);
extern SEXP _coolboxr_metadata_version(void);
extern SEXP _coolboxr_metadata_describe(void);
extern SEXP _coolboxr_metadata_capability_count(void);
extern SEXP _coolboxr_metadata_capability_at(SEXP);
extern SEXP _coolboxr_metadata_is_ready(void);
extern SEXP _coolboxr_uuid_v1(void);
extern SEXP _coolboxr_uuid_v2(SEXP, SEXP);
extern SEXP _coolboxr_uuid_v3(SEXP, SEXP);
extern SEXP _coolboxr_uuid_v4(void);
extern SEXP _coolboxr_uuid_v5(SEXP, SEXP);
extern SEXP _coolboxr_uuid_v6(void);
extern SEXP _coolboxr_uuid_v8(SEXP);
extern SEXP _coolboxr_guid(void);

static const R_CallMethodDef CallEntries[] = {
    {"_coolboxr_fit_linear_regression", (DL_FUNC) &_coolboxr_fit_linear_regression, 5},
    {"_coolboxr_predict_linear_regression", (DL_FUNC) &_coolboxr_predict_linear_regression, 2},
    {"_coolboxr_time_series_summary", (DL_FUNC) &_coolboxr_time_series_summary, 2},
    {"_coolboxr_transform_time_series", (DL_FUNC) &_coolboxr_transform_time_series, 5},
    {"_coolboxr_forecast_time_series", (DL_FUNC) &_coolboxr_forecast_time_series, 6},
    {"_coolboxr_detect_time_series_outliers", (DL_FUNC) &_coolboxr_detect_time_series_outliers, 3},
    {"_coolboxr_fit_decision_tree", (DL_FUNC) &_coolboxr_fit_decision_tree, 4},
    {"_coolboxr_predict_decision_tree", (DL_FUNC) &_coolboxr_predict_decision_tree, 2},
    {"_coolboxr_fit_svm", (DL_FUNC) &_coolboxr_fit_svm, 6},
    {"_coolboxr_predict_svm", (DL_FUNC) &_coolboxr_predict_svm, 2},
    {"_coolboxr_create_bayesian_network", (DL_FUNC) &_coolboxr_create_bayesian_network, 0},
    {"_coolboxr_bn_add_node", (DL_FUNC) &_coolboxr_bn_add_node, 3},
    {"_coolboxr_bn_add_edge", (DL_FUNC) &_coolboxr_bn_add_edge, 3},
    {"_coolboxr_bn_set_cpt", (DL_FUNC) &_coolboxr_bn_set_cpt, 3},
    {"_coolboxr_bn_get_node_id", (DL_FUNC) &_coolboxr_bn_get_node_id, 2},
    {"_coolboxr_bn_query", (DL_FUNC) &_coolboxr_bn_query, 4},
    {"_coolboxr_bn_joint_probability", (DL_FUNC) &_coolboxr_bn_joint_probability, 2},
    {"_coolboxr_bn_nodes", (DL_FUNC) &_coolboxr_bn_nodes, 1},
    {"_coolboxr_create_hmm", (DL_FUNC) &_coolboxr_create_hmm, 2},
    {"_coolboxr_hmm_set_parameters", (DL_FUNC) &_coolboxr_hmm_set_parameters, 4},
    {"_coolboxr_hmm_get_parameters", (DL_FUNC) &_coolboxr_hmm_get_parameters, 1},
    {"_coolboxr_hmm_train", (DL_FUNC) &_coolboxr_hmm_train, 6},
    {"_coolboxr_hmm_viterbi", (DL_FUNC) &_coolboxr_hmm_viterbi, 2},
    {"_coolboxr_hmm_log_likelihood", (DL_FUNC) &_coolboxr_hmm_log_likelihood, 2},
    {"_coolboxr_run_bandit_simulation", (DL_FUNC) &_coolboxr_run_bandit_simulation, 7},
    {"_coolboxr_create_marked_point_process", (DL_FUNC) &_coolboxr_create_marked_point_process, 3},
    {"_coolboxr_mpp_fit", (DL_FUNC) &_coolboxr_mpp_fit, 3},
    {"_coolboxr_mpp_predict_intensity", (DL_FUNC) &_coolboxr_mpp_predict_intensity, 4},
    {"_coolboxr_mpp_generate_sequence", (DL_FUNC) &_coolboxr_mpp_generate_sequence, 3},
    {"_coolboxr_mpp_log_likelihood", (DL_FUNC) &_coolboxr_mpp_log_likelihood, 3},
    {"_coolboxr_mpp_parameters", (DL_FUNC) &_coolboxr_mpp_parameters, 1},
    {"_coolboxr_create_pcim", (DL_FUNC) &_coolboxr_create_pcim, 3},
    {"_coolboxr_pcim_create_uniform_intervals", (DL_FUNC) &_coolboxr_pcim_create_uniform_intervals, 4},
    {"_coolboxr_pcim_create_adaptive_intervals", (DL_FUNC) &_coolboxr_pcim_create_adaptive_intervals, 3},
    {"_coolboxr_pcim_fit", (DL_FUNC) &_coolboxr_pcim_fit, 2},
    {"_coolboxr_pcim_fit_with_covariates", (DL_FUNC) &_coolboxr_pcim_fit_with_covariates, 3},
    {"_coolboxr_pcim_predict_intensity", (DL_FUNC) &_coolboxr_pcim_predict_intensity, 3},
    {"_coolboxr_pcim_predict_intensity_with_covariates", (DL_FUNC) &_coolboxr_pcim_predict_intensity_with_covariates, 4},
    {"_coolboxr_pcim_generate_sequence", (DL_FUNC) &_coolboxr_pcim_generate_sequence, 3},
    {"_coolboxr_pcim_log_likelihood", (DL_FUNC) &_coolboxr_pcim_log_likelihood, 2},
    {"_coolboxr_pcim_information_criteria", (DL_FUNC) &_coolboxr_pcim_information_criteria, 2},
    {"_coolboxr_pcim_intervals", (DL_FUNC) &_coolboxr_pcim_intervals, 1},
    {"_coolboxr_fit_latent_sentiment_analysis", (DL_FUNC) &_coolboxr_fit_latent_sentiment_analysis, 5},
    {"_coolboxr_lsa_predict_score", (DL_FUNC) &_coolboxr_lsa_predict_score, 3},
    {"_coolboxr_lsa_factors", (DL_FUNC) &_coolboxr_lsa_factors, 1},
    {"_coolboxr_fit_svd", (DL_FUNC) &_coolboxr_fit_svd, 2},
    {"_coolboxr_svd_summary", (DL_FUNC) &_coolboxr_svd_summary, 1},
    {"_coolboxr_svd_reconstruct", (DL_FUNC) &_coolboxr_svd_reconstruct, 2},
    {"_coolboxr_fit_pca", (DL_FUNC) &_coolboxr_fit_pca, 4},
    {"_coolboxr_pca_transform", (DL_FUNC) &_coolboxr_pca_transform, 2},
    {"_coolboxr_pca_inverse_transform", (DL_FUNC) &_coolboxr_pca_inverse_transform, 2},
    {"_coolboxr_pca_summary", (DL_FUNC) &_coolboxr_pca_summary, 1},
    {"_coolboxr_fit_knn", (DL_FUNC) &_coolboxr_fit_knn, 3},
    {"_coolboxr_knn_kneighbors", (DL_FUNC) &_coolboxr_knn_kneighbors, 2},
    {"_coolboxr_knn_pairwise_distances", (DL_FUNC) &_coolboxr_knn_pairwise_distances, 3},
    {"_coolboxr_fit_umap", (DL_FUNC) &_coolboxr_fit_umap, 8},
    {"_coolboxr_umap_transform", (DL_FUNC) &_coolboxr_umap_transform, 2},
    {"_coolboxr_umap_embedding", (DL_FUNC) &_coolboxr_umap_embedding, 1},
    {"_coolboxr_metadata_version", (DL_FUNC) &_coolboxr_metadata_version, 0},
    {"_coolboxr_metadata_describe", (DL_FUNC) &_coolboxr_metadata_describe, 0},
    {"_coolboxr_metadata_capability_count", (DL_FUNC) &_coolboxr_metadata_capability_count, 0},
    {"_coolboxr_metadata_capability_at", (DL_FUNC) &_coolboxr_metadata_capability_at, 1},
    {"_coolboxr_metadata_is_ready", (DL_FUNC) &_coolboxr_metadata_is_ready, 0},
    {"_coolboxr_uuid_v1", (DL_FUNC) &_coolboxr_uuid_v1, 0},
    {"_coolboxr_uuid_v2", (DL_FUNC) &_coolboxr_uuid_v2, 2},
    {"_coolboxr_uuid_v3", (DL_FUNC) &_coolboxr_uuid_v3, 2},
    {"_coolboxr_uuid_v4", (DL_FUNC) &_coolboxr_uuid_v4, 0},
    {"_coolboxr_uuid_v5", (DL_FUNC) &_coolboxr_uuid_v5, 2},
    {"_coolboxr_uuid_v6", (DL_FUNC) &_coolboxr_uuid_v6, 0},
    {"_coolboxr_uuid_v8", (DL_FUNC) &_coolboxr_uuid_v8, 1},
    {"_coolboxr_guid", (DL_FUNC) &_coolboxr_guid, 0},
    {NULL, NULL, 0}
};

void R_init_coolboxr(DllInfo* dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
