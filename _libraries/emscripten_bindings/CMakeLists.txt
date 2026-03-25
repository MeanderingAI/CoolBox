cmake_minimum_required(VERSION 3.12)
project(emscripten_bindings)

# Root of the CoolBox project (two levels up from this CMakeLists.txt)
get_filename_component(COOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Option to control MODULARIZE
option(EMSCRIPTEN_MODULARIZE "Build with MODULARIZE=1" ON)

# Fetch Eigen (needed by curcuitry/circuit_solver)
include(FetchContent)
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    TRUE
)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(eigen)

# ── advanced_logging source (compiled directly for Emscripten) ──────
set(ADV_LOG_SRC_DIR "${COOLBOX_ROOT}/_libraries/backages/IO/advanced_logging")

add_executable(advanced_logging_js
    advanced_logging_bindings.cpp
    ${ADV_LOG_SRC_DIR}/source/advanced_logging.cpp
)

target_include_directories(advanced_logging_js PRIVATE
    ${COOLBOX_ROOT}/_libraries/include
    ${ADV_LOG_SRC_DIR}/headers
)

# Emscripten-specific flags
if(EMSCRIPTEN)
    set_target_properties(advanced_logging_js PROPERTIES
        OUTPUT_NAME "advanced_logging"
        SUFFIX ".js"
    )
    if(EMSCRIPTEN_MODULARIZE)
        target_link_options(advanced_logging_js PRIVATE
            "SHELL:-s MODULARIZE=1"
            "SHELL:-s EXPORT_NAME='createAdvancedLoggingModule'"
            "--bind"
        )
    else()
        target_link_options(advanced_logging_js PRIVATE
            "SHELL:-s MODULARIZE=0"
            "--bind"
        )
    endif()
else()
    message(WARNING "Not building with Emscripten — JS output will not be produced.")
endif()

# ── curcuitry bindings ──────────────────────────────────────────────
set(CURCUITRY_SRC_DIR "${COOLBOX_ROOT}/_libraries/backages/ELECTRONICS/curcuitry")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/curcuitry_bindings.cpp")
    add_executable(curcuitry_js
        curcuitry_bindings.cpp
        ${CURCUITRY_SRC_DIR}/src/circuit.cpp
    )
    target_include_directories(curcuitry_js PRIVATE
        ${COOLBOX_ROOT}/_libraries/include
        ${CURCUITRY_SRC_DIR}/include
    )
    target_link_libraries(curcuitry_js PRIVATE Eigen3::Eigen)
    if(EMSCRIPTEN)
        set_target_properties(curcuitry_js PROPERTIES
            OUTPUT_NAME "curcuitry"
            SUFFIX ".js"
        )
        if(EMSCRIPTEN_MODULARIZE)
            target_link_options(curcuitry_js PRIVATE
                "SHELL:-s MODULARIZE=1"
                "SHELL:-s EXPORT_NAME='createCurcuitryModule'"
                "--bind"
            )
        else()
            target_link_options(curcuitry_js PRIVATE "--bind")
        endif()
    endif()
endif()

# ── battery bindings ────────────────────────────────────────────────
set(BATTERY_SRC_DIR "${COOLBOX_ROOT}/_libraries/backages/ELECTRONICS/battery")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/battery_bindings.cpp")
    add_executable(battery_js
        battery_bindings.cpp
    )
    target_include_directories(battery_js PRIVATE
        ${COOLBOX_ROOT}/_libraries/include
        ${BATTERY_SRC_DIR}/include
        ${CURCUITRY_SRC_DIR}/include
    )
    if(EMSCRIPTEN)
        set_target_properties(battery_js PROPERTIES
            OUTPUT_NAME "battery"
            SUFFIX ".js"
        )
        if(EMSCRIPTEN_MODULARIZE)
            target_link_options(battery_js PRIVATE
                "SHELL:-s MODULARIZE=1"
                "SHELL:-s EXPORT_NAME='createBatteryModule'"
                "--bind"
            )
        else()
            target_link_options(battery_js PRIVATE "--bind")
        endif()
    endif()
endif()

# ── data_structures bindings ────────────────────────────────────────
set(DS_SRC_DIR "${COOLBOX_ROOT}/_libraries/backages/DATASTRUCTURE")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data_structures_bindings.cpp")
    add_executable(data_structures_js
        data_structures_bindings.cpp
        ${DS_SRC_DIR}/trees/source/binary_search_tree.cpp
        ${DS_SRC_DIR}/list/source/linked_list.cpp
        ${DS_SRC_DIR}/maps/source/hash_map.cpp
    )
    target_include_directories(data_structures_js PRIVATE
        ${DS_SRC_DIR}/headers
        ${DS_SRC_DIR}/trees/headers
        ${DS_SRC_DIR}/maps/headers
        ${DS_SRC_DIR}/list/headers
        ${DS_SRC_DIR}/filters/headers
    )
    if(EMSCRIPTEN)
        set_target_properties(data_structures_js PROPERTIES
            OUTPUT_NAME "data_structures"
            SUFFIX ".js"
        )
        if(EMSCRIPTEN_MODULARIZE)
            target_link_options(data_structures_js PRIVATE
                "SHELL:-s MODULARIZE=1"
                "SHELL:-s EXPORT_NAME='createDataStructuresModule'"
                "--bind"
            )
        else()
            target_link_options(data_structures_js PRIVATE "--bind")
        endif()
    endif()
endif()

# ── gabor_patches bindings ──────────────────────────────────────────
set(GP_SRC_DIR "${COOLBOX_ROOT}/_libraries/backages/ML/gabor_patches")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gabor_patches_bindings.cpp")
    add_executable(gabor_patches_js
        gabor_patches_bindings.cpp
        ${GP_SRC_DIR}/source/gabor_patches.cpp
    )
    target_include_directories(gabor_patches_js PRIVATE
        ${GP_SRC_DIR}/headers
    )
    target_link_libraries(gabor_patches_js PRIVATE Eigen3::Eigen)
    if(EMSCRIPTEN)
        set_target_properties(gabor_patches_js PROPERTIES
            OUTPUT_NAME "gabor_patches"
            SUFFIX ".js"
        )
        if(EMSCRIPTEN_MODULARIZE)
            target_link_options(gabor_patches_js PRIVATE
                "SHELL:-s MODULARIZE=1"
                "SHELL:-s EXPORT_NAME='createGaborPatchesModule'"
                "--bind"
            )
        else()
            target_link_options(gabor_patches_js PRIVATE "--bind")
        endif()
    endif()
endif()

# Global include for lib_metadata.h and other shared headers
set(BACKAGES_ROOT "${COOLBOX_ROOT}/_libraries/backages")
set(METADATA_HEADERS_DIR "${COOLBOX_ROOT}/_libraries/backages/MISC/metadata_management/headers")

# ===================================================================
# Helper macro: add an emscripten JS module from a bindings file + sources
# ===================================================================
macro(add_emscripten_module NAME EXPORT_NAME BINDING_FILE SOURCES INCLUDE_DIRS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BINDING_FILE}")
        add_executable(${NAME} ${BINDING_FILE} ${SOURCES})
        target_include_directories(${NAME} PRIVATE ${INCLUDE_DIRS} ${METADATA_HEADERS_DIR} ${BACKAGES_ROOT} ${COOLBOX_ROOT}/_libraries/include)
        if(EMSCRIPTEN)
            set_target_properties(${NAME} PROPERTIES SUFFIX ".js")
            if(EMSCRIPTEN_MODULARIZE)
                target_link_options(${NAME} PRIVATE
                    "SHELL:-s MODULARIZE=1"
                    "SHELL:-s EXPORT_NAME='${EXPORT_NAME}'"
                    "--bind"
                )
            else()
                target_link_options(${NAME} PRIVATE "--bind")
            endif()
        endif()
    endif()
endmacro()

# ── ML library source directories ──────────────────────────────────
set(ML_ROOT "${COOLBOX_ROOT}/_libraries/backages/ML")

# ── Decision Tree ──────────────────────────────────────────────────
add_emscripten_module(decision_tree_js createDecisionTreeModule
    decision_tree_bindings.cpp
    "${ML_ROOT}/decision_tree/source/decision_tree.cpp;${ML_ROOT}/decision_tree/source/random_forest.cpp;${ML_ROOT}/decision_tree/source/boost_tree.cpp;${ML_ROOT}/decision_tree/source/rule_set.cpp"
    "${ML_ROOT}/decision_tree/headers"
)
if(TARGET decision_tree_js)
    target_link_libraries(decision_tree_js PRIVATE Eigen3::Eigen)
endif()

# ── Bayesian Network ──────────────────────────────────────────────
add_emscripten_module(bayesian_network_js createBayesianNetworkModule
    bayesian_network_bindings.cpp
    "${ML_ROOT}/bayesian_network_ai/source/bayesian_network.cpp"
    "${ML_ROOT}/bayesian_network_ai/headers"
)

# ── Hidden Markov Model ──────────────────────────────────────────
add_emscripten_module(hidden_markov_model_js createHMMModule
    hidden_markov_model_bindings.cpp
    "${ML_ROOT}/hidden_markov_model/source/hidden_markov_model.cpp"
    "${ML_ROOT}/hidden_markov_model/headers"
)
if(TARGET hidden_markov_model_js)
    target_link_libraries(hidden_markov_model_js PRIVATE Eigen3::Eigen)
endif()

# ── Distributions ────────────────────────────────────────────────
add_emscripten_module(distribution_js createDistributionModule
    distribution_bindings.cpp
    "${ML_ROOT}/distribution/source/normal_distribution.cpp;${ML_ROOT}/distribution/source/bernoulli_distribution.cpp;${ML_ROOT}/distribution/source/binomial_distribution.cpp;${ML_ROOT}/distribution/source/poisson_distribution.cpp;${ML_ROOT}/distribution/source/exponential_distribution.cpp;${ML_ROOT}/distribution/source/gamma_distribution.cpp;${ML_ROOT}/distribution/source/discrete_distribution.cpp;${ML_ROOT}/distribution/source/util.cpp"
    "${ML_ROOT}/distribution/headers"
)
if(TARGET distribution_js)
    target_link_libraries(distribution_js PRIVATE Eigen3::Eigen)
endif()

# ── Generalized Linear Model ────────────────────────────────────
add_emscripten_module(glm_js createGLMModule
    glm_bindings.cpp
    "${ML_ROOT}/generalized_linear_model/source/generalized_linear_model.cpp;${ML_ROOT}/generalized_linear_model/source/linear_regression.cpp"
    "${ML_ROOT}/generalized_linear_model/headers;${ML_ROOT}/distribution/headers"
)
if(TARGET glm_js)
    target_link_libraries(glm_js PRIVATE Eigen3::Eigen)
endif()

# ── Multi-Arm Bandit ─────────────────────────────────────────────
add_emscripten_module(multi_arm_bandit_js createMultiArmBanditModule
    multi_arm_bandit_bindings.cpp
    "${ML_ROOT}/multi_arm_bandit/source/bandit_arm.cpp;${ML_ROOT}/multi_arm_bandit/source/bandit_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/epsilon_greedy_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/ucb_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/thompson_sampling_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/decaying_epsilon_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/simulation_result.cpp"
    "${ML_ROOT}/multi_arm_bandit/headers"
)

# ── Deep Learning ────────────────────────────────────────────────
add_emscripten_module(deep_learning_js createDeepLearningModule
    deep_learning_bindings.cpp
    "${ML_ROOT}/deep_learning/source/tensor.cpp;${ML_ROOT}/deep_learning/source/layer.cpp;${ML_ROOT}/deep_learning/source/loss.cpp;${ML_ROOT}/deep_learning/source/optimizer.cpp;${ML_ROOT}/deep_learning/source/neural_network.cpp;${ML_ROOT}/deep_learning/source/templates.cpp"
    "${ML_ROOT}/deep_learning/headers"
)
if(TARGET deep_learning_js)
    target_link_libraries(deep_learning_js PRIVATE Eigen3::Eigen)
endif()

# ── Support Vector Machine ───────────────────────────────────────
add_emscripten_module(svm_js createSVMModule
    svm_bindings.cpp
    "${ML_ROOT}/support_vector_machine/source/support_vector_machine.cpp;${ML_ROOT}/support_vector_machine/source/linear_kernel.cpp;${ML_ROOT}/support_vector_machine/source/rbf_kernel.cpp;${ML_ROOT}/support_vector_machine/source/polynomial_kernel.cpp;${ML_ROOT}/support_vector_machine/source/sigmoid_kernel.cpp"
    "${ML_ROOT}/support_vector_machine/headers"
)
if(TARGET svm_js)
    target_link_libraries(svm_js PRIVATE Eigen3::Eigen)
endif()

# ── Dimensionality Reduction (PCA) ──────────────────────────────
add_emscripten_module(dimensionality_reduction_js createDimReducModule
    dimensionality_reduction_bindings.cpp
    "${ML_ROOT}/dimensionality_reduction/source/pca.cpp;${ML_ROOT}/dimensionality_reduction/source/svd.cpp;${ML_ROOT}/dimensionality_reduction/source/knn.cpp;${ML_ROOT}/dimensionality_reduction/source/umap.cpp"
    "${ML_ROOT}/dimensionality_reduction/headers"
)
if(TARGET dimensionality_reduction_js)
    target_link_libraries(dimensionality_reduction_js PRIVATE Eigen3::Eigen)
endif()

# ── Latent Sentiment Analysis ────────────────────────────────────
add_emscripten_module(latent_sentiment_analysis_js createLSAModule
    latent_sentiment_analysis_bindings.cpp
    "${ML_ROOT}/latent_sentiment_analysis/source/latent_sentiment_analysis.cpp"
    "${ML_ROOT}/latent_sentiment_analysis/headers"
)
if(TARGET latent_sentiment_analysis_js)
    target_link_libraries(latent_sentiment_analysis_js PRIVATE Eigen3::Eigen)
endif()

# ── Marked Point Process ────────────────────────────────────────
add_emscripten_module(marked_point_process_js createMPPModule
    marked_point_process_bindings.cpp
    "${ML_ROOT}/marked_point_process/source/marked_point_process.cpp;${ML_ROOT}/marked_point_process/source/piecewise_conditional_intensity_model.cpp"
    "${ML_ROOT}/marked_point_process/headers"
)
if(TARGET marked_point_process_js)
    target_link_libraries(marked_point_process_js PRIVATE Eigen3::Eigen)
endif()

# ── Tracker (Kalman Filters) ────────────────────────────────────
add_emscripten_module(tracker_js createTrackerModule
    tracker_bindings.cpp
    "${ML_ROOT}/tracker/source/kalman_filter.cpp;${ML_ROOT}/tracker/source/extended_kalman_filter.cpp;${ML_ROOT}/tracker/source/unscented_kalman_filter.cpp;${ML_ROOT}/tracker/source/sequential_monte_carlo.cpp"
    "${ML_ROOT}/tracker/headers"
)
if(TARGET tracker_js)
    target_link_libraries(tracker_js PRIVATE Eigen3::Eigen)
endif()

# ── Time Series ─────────────────────────────────────────────────
add_emscripten_module(time_series_js createTimeSeriesModule
    time_series_bindings.cpp
    "${ML_ROOT}/time_series/source/time_series.cpp"
    "${ML_ROOT}/time_series/headers"
)
if(TARGET time_series_js)
    target_link_libraries(time_series_js PRIVATE Eigen3::Eigen)
endif()
