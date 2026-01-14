# ML/AI Tests
add_executable(tracker_tests
    test_extended_kalman_filter.cpp
    test_kalman_filter.cpp
    test_sequential_monte_carlo.cpp
    test_unscented_kalman_filter.cpp)
target_link_libraries(tracker_tests PRIVATE tracker Eigen3::Eigen gtest_main)
target_include_directories(tracker_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TrackerTests COMMAND tracker_tests)

add_executable(lsa_tests test_latent_sentiment_analysis.cpp)
target_link_libraries(lsa_tests PRIVATE latent_sentiment_analysis Eigen3::Eigen gtest_main)
target_include_directories(lsa_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/latent_sentiment_analysis)
add_test(NAME LSATests COMMAND lsa_tests)

add_executable(hmm_tests test_hidden_markov_model.cpp)
target_link_libraries(hmm_tests PRIVATE hidden_markov_model Eigen3::Eigen gtest_main)
target_include_directories(hmm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME HMMTests COMMAND hmm_tests)

add_executable(bayesian_network_tests test_baysian_network.cpp)
target_link_libraries(bayesian_network_tests PRIVATE bayesian_network Eigen3::Eigen gtest_main)
target_include_directories(bayesian_network_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME BayesianNetworkTests COMMAND bayesian_network_tests)

add_executable(decision_tree_tests
    test_decision_tree.cpp
    test_rule_set.cpp
    test_random_forest.cpp
    test_boost_tree.cpp)
target_link_libraries(decision_tree_tests PRIVATE decision_tree Eigen3::Eigen gtest_main)
target_include_directories(decision_tree_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DecisionTreeTests COMMAND decision_tree_tests)

add_executable(glm_tests test_linear_regression.cpp)
target_link_libraries(glm_tests PRIVATE generalized_linear_model Eigen3::Eigen gtest_main)
target_include_directories(glm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME GLMTests COMMAND glm_tests)

add_executable(distribution_tests test_distribution.cpp)
target_link_libraries(distribution_tests PRIVATE probability_distribution Eigen3::Eigen gtest_main)
target_include_directories(distribution_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributionTests COMMAND distribution_tests)

add_executable(mab_tests test_multi_arm_bandit.cpp)
target_link_libraries(mab_tests PRIVATE multi_arm_bandit Eigen3::Eigen gtest_main)
target_include_directories(mab_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MABTests COMMAND mab_tests)

add_executable(svm_tests test_support_vector_machine.cpp)
target_link_libraries(svm_tests PRIVATE support_vector_machine Eigen3::Eigen gtest_main)
target_include_directories(svm_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/support_vector_machine)
add_test(NAME SVMTests COMMAND svm_tests)

add_executable(mpp_tests test_marked_point_process.cpp)
target_link_libraries(mpp_tests PRIVATE marked_point_process Eigen3::Eigen gtest_main)
target_include_directories(mpp_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/marked_point_process)
add_test(NAME MPPTests COMMAND mpp_tests)

add_executable(pcim_tests test_piecewise_conditional_intensity_model.cpp)
target_link_libraries(pcim_tests PRIVATE marked_point_process Eigen3::Eigen gtest_main)
target_include_directories(pcim_tests PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include/marked_point_process)
add_test(NAME PCIMTests COMMAND pcim_tests)

add_executable(svd_tests test_svd.cpp)
target_link_libraries(svd_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(svd_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SVDTests COMMAND svd_tests)

add_executable(pca_tests test_pca.cpp)
target_link_libraries(pca_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(pca_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PCATests COMMAND pca_tests)

add_executable(knn_tests test_knn.cpp)
target_link_libraries(knn_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(knn_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME KNNTests COMMAND knn_tests)

add_executable(umap_tests test_umap.cpp)
target_link_libraries(umap_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(umap_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME UMAPTests COMMAND umap_tests)
