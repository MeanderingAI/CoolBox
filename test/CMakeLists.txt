enable_testing()

set(FILTER_SOURCES
    test_extended_kalman_filter.cpp
    test_kalman_filter.cpp
    test_sequential_monte_carlo.cpp
    test_unscented_kalman_filter.cpp
)
add_executable(tracker_tests ${FILTER_SOURCES})
target_link_libraries(tracker_tests PRIVATE tracker Eigen3::Eigen gtest_main)
target_include_directories(tracker_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TrackerTests COMMAND tracker_tests)

set(LSA_SOURCES
    test_latent_sentiment_analysis.cpp
)
add_executable(lsa_tests ${LSA_SOURCES})
target_link_libraries(lsa_tests PRIVATE latent_sentiment_analysis Eigen3::Eigen gtest_main)
target_include_directories(lsa_tests PRIVATE ${CMAKE_SOURCE_DIR}/include/latent_sentiment_analysis)
add_test(NAME LSATests COMMAND lsa_tests)

set(HMM_SOURCES
    test_hidden_markov_model.cpp
)
add_executable(hmm_tests ${HMM_SOURCES})
target_link_libraries(hmm_tests PRIVATE hidden_markov_model Eigen3::Eigen gtest_main)
target_include_directories(hmm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME HMMTests COMMAND hmm_tests)

set(BAYESIAN_NETWORK_SOURCES
    test_baysian_network.cpp
)
add_executable(bayesian_network_tests ${BAYESIAN_NETWORK_SOURCES})
target_link_libraries(bayesian_network_tests PRIVATE bayesian_network Eigen3::Eigen gtest_main)
target_include_directories(bayesian_network_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME BayesianNetworkTests COMMAND bayesian_network_tests)

set(TREE_SOURCES
    test_decision_tree.cpp
    test_rule_set.cpp
    test_random_forest.cpp
    test_boost_tree.cpp
)
add_executable(decision_tree_tests ${TREE_SOURCES})
target_link_libraries(decision_tree_tests PRIVATE decision_tree Eigen3::Eigen gtest_main)
target_include_directories(decision_tree_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DecisionTreeTests COMMAND decision_tree_tests)

set(GLM_SOURCES
    test_linear_regression.cpp
)
add_executable(glm_tests ${GLM_SOURCES})
target_link_libraries(glm_tests PRIVATE generalized_linear_model Eigen3::Eigen gtest_main)
target_include_directories(glm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME GLMTests COMMAND glm_tests)

set(DISTRIBUTION_SOURCES
    test_distribution.cpp
)
add_executable(distribution_tests ${DISTRIBUTION_SOURCES})
target_link_libraries(distribution_tests PRIVATE probability_distribution Eigen3::Eigen gtest_main)
target_include_directories(distribution_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributionTests COMMAND distribution_tests)

set(MAB_SOURCES
    test_multi_arm_bandit.cpp
)
add_executable(mab_tests ${MAB_SOURCES})
target_link_libraries(mab_tests PRIVATE multi_arm_bandit Eigen3::Eigen gtest_main)
target_include_directories(mab_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MABTests COMMAND mab_tests)

set(SVM_SOURCES
    test_support_vector_machine.cpp
)
add_executable(svm_tests ${SVM_SOURCES})
target_link_libraries(svm_tests PRIVATE support_vector_machine Eigen3::Eigen gtest_main)
target_include_directories(svm_tests PRIVATE ${CMAKE_SOURCE_DIR}/include/support_vector_machine)
add_test(NAME SVMTests COMMAND svm_tests)

set(MPP_SOURCES
    test_marked_point_process.cpp
)
add_executable(mpp_tests ${MPP_SOURCES})
target_link_libraries(mpp_tests PRIVATE marked_point_process Eigen3::Eigen gtest_main)
target_include_directories(mpp_tests PRIVATE ${CMAKE_SOURCE_DIR}/include/marked_point_process)
add_test(NAME MPPTests COMMAND mpp_tests)

set(PCIM_SOURCES
    test_piecewise_conditional_intensity_model.cpp
)
add_executable(pcim_tests ${PCIM_SOURCES})
target_link_libraries(pcim_tests PRIVATE marked_point_process Eigen3::Eigen gtest_main)
target_include_directories(pcim_tests PRIVATE ${CMAKE_SOURCE_DIR}/include/marked_point_process)
add_test(NAME PCIMTests COMMAND pcim_tests)

set(SVD_SOURCES
    test_svd.cpp
)
add_executable(svd_tests ${SVD_SOURCES})
target_link_libraries(svd_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(svd_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SVDTests COMMAND svd_tests)

set(PCA_SOURCES
    test_pca.cpp
)
add_executable(pca_tests ${PCA_SOURCES})
target_link_libraries(pca_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(pca_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PCATests COMMAND pca_tests)

set(KNN_SOURCES
    test_knn.cpp
)
add_executable(knn_tests ${KNN_SOURCES})
target_link_libraries(knn_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(knn_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME KNNTests COMMAND knn_tests)

set(UMAP_SOURCES
    test_umap.cpp
)
add_executable(umap_tests ${UMAP_SOURCES})
target_link_libraries(umap_tests PRIVATE dimensionality_reduction Eigen3::Eigen gtest_main)
target_include_directories(umap_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME UMAPTests COMMAND umap_tests)

# JSON library tests
add_executable(json_tests test_json.cpp)
target_link_libraries(json_tests PRIVATE json gtest_main pthread)
target_include_directories(json_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME JsonTests COMMAND json_tests)

# REST API library tests (TEMPORARILY DISABLED - API changed)
# add_executable(rest_api_tests test_rest_api.cpp)
# target_link_libraries(rest_api_tests PRIVATE rest_api json gtest_main pthread)
# target_include_directories(rest_api_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
# add_test(NAME RestApiTests COMMAND rest_api_tests)

# SQL library tests (disabled - API changed)
# add_executable(sql_tests test_sql.cpp)
# target_link_libraries(sql_tests PRIVATE sql sqlite3 gtest_main pthread)
# target_include_directories(sql_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
# add_test(NAME SqlTests COMMAND sql_tests)

# Data structures library tests
add_executable(data_structures_tests 
    test_binary_search_tree.cpp
    test_linked_list.cpp
    test_hash_map.cpp
    test_concurrent_hash_map.cpp
    test_concurrent_linked_list.cpp
)
target_link_libraries(data_structures_tests PRIVATE data_structures gtest_main pthread)
target_include_directories(data_structures_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DataStructuresTests COMMAND data_structures_tests)

# Distributed file system tests
add_executable(distributed_fs_tests test_distributed_fs.cpp)
target_link_libraries(distributed_fs_tests PRIVATE distributed_fs gtest_main pthread)
target_include_directories(distributed_fs_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributedFSTests COMMAND distributed_fs_tests)
