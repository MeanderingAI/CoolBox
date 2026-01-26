# ML/AI Tests
add_executable(tracker_tests
    test_extended_kalman_filter.cpp
    test_kalman_filter.cpp
    test_sequential_monte_carlo.cpp
    test_unscented_kalman_filter.cpp)
set(TRACKER_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/tracker/libtracker.dylib")
set(GTEST_MAIN_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest_main.a")
set(GTEST_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest.a")
target_link_libraries(tracker_tests PRIVATE ${TRACKER_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(tracker_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TrackerTests COMMAND tracker_tests)


set(LSA_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/latent_sentiment_analysis/liblatent_sentiment_analysis.dylib")
add_executable(lsa_tests test_latent_sentiment_analysis.cpp)
target_link_libraries(lsa_tests PRIVATE ${LSA_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(lsa_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/latent_sentiment_analysis)
add_test(NAME LSATests COMMAND lsa_tests)


set(HMM_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/hidden_markov_model/libhidden_markov_model.dylib")
add_executable(hmm_tests test_hidden_markov_model.cpp)
target_link_libraries(hmm_tests PRIVATE ${HMM_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(hmm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME HMMTests COMMAND hmm_tests)


set(BAYESIAN_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/bayesian_network/libbayesian_network.dylib")
add_executable(bayesian_network_tests test_baysian_network.cpp)
target_link_libraries(bayesian_network_tests PRIVATE ${BAYESIAN_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(bayesian_network_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME BayesianNetworkTests COMMAND bayesian_network_tests)


set(DECISION_TREE_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/decision_tree/libdecision_tree.dylib")
add_executable(decision_tree_tests
    test_decision_tree.cpp
    test_rule_set.cpp
    test_random_forest.cpp
    test_boost_tree.cpp)
target_link_libraries(decision_tree_tests PRIVATE ${DECISION_TREE_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(decision_tree_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DecisionTreeTests COMMAND decision_tree_tests)


set(GLM_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/generalized_linear_model/libgeneralized_linear_model.dylib")
add_executable(glm_tests test_linear_regression.cpp)
target_link_libraries(glm_tests PRIVATE ${GLM_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(glm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME GLMTests COMMAND glm_tests)


set(DISTRIBUTION_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/distribution/libprobability_distribution.dylib")
add_executable(distribution_tests test_distribution.cpp)
target_link_libraries(distribution_tests PRIVATE ${DISTRIBUTION_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(distribution_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributionTests COMMAND distribution_tests)


set(MAB_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/multi_arm_bandit/libmulti_arm_bandit.dylib")
add_executable(mab_tests test_multi_arm_bandit.cpp)
target_link_libraries(mab_tests PRIVATE ${MAB_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(mab_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MABTests COMMAND mab_tests)


set(SVM_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/support_vector_machine/libsupport_vector_machine.dylib")
add_executable(svm_tests test_support_vector_machine.cpp)
target_link_libraries(svm_tests PRIVATE ${SVM_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(svm_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/support_vector_machine)
add_test(NAME SVMTests COMMAND svm_tests)


set(MPP_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ml/marked_point_process/libmarked_point_process.dylib")
add_executable(mpp_tests test_marked_point_process.cpp)
target_link_libraries(mpp_tests PRIVATE ${MPP_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(mpp_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/marked_point_process)
add_test(NAME MPPTests COMMAND mpp_tests)


set(PCIM_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ML/marked_point_process/libmarked_point_process.dylib")
add_executable(pcim_tests test_piecewise_conditional_intensity_model.cpp)
target_link_libraries(pcim_tests PRIVATE ${PCIM_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(pcim_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/marked_point_process)
add_test(NAME PCIMTests COMMAND pcim_tests)


set(SVD_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ML/dimensionality_reduction/libdimensionality_reduction.dylib")
add_executable(svd_tests test_svd.cpp)
target_link_libraries(svd_tests PRIVATE ${SVD_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(svd_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SVDTests COMMAND svd_tests)


set(PCA_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ML/dimensionality_reduction/libdimensionality_reduction.dylib")
add_executable(pca_tests test_pca.cpp)
target_link_libraries(pca_tests PRIVATE ${PCA_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(pca_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PCATests COMMAND pca_tests)


set(KNN_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ML/dimensionality_reduction/libdimensionality_reduction.dylib")
add_executable(knn_tests test_knn.cpp)
target_link_libraries(knn_tests PRIVATE ${KNN_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(knn_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME KNNTests COMMAND knn_tests)


set(UMAP_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/ML/dimensionality_reduction/libdimensionality_reduction.dylib")
add_executable(umap_tests test_umap.cpp)
target_link_libraries(umap_tests PRIVATE ${UMAP_LIB_PATH} Eigen3::Eigen ${GTEST_MAIN_PATH} ${GTEST_PATH})
target_include_directories(umap_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME UMAPTests COMMAND umap_tests)
