cmake_minimum_required(VERSION 3.12)
project(emscripten_bindings)

include(FetchContent)

# Root of the CoolBox project (four levels up from this CMakeLists.txt)
get_filename_component(COOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

function(resolve_eigen3_include_dir OUTPUT_VAR)
    set(_eigen_candidates
        "${COOLBOX_ROOT}/external/eigen/eigen-3.4.0"
        "${COOLBOX_ROOT}/build/_deps/eigen-src"
        "/usr/include/eigen3"
        "/opt/homebrew/include/eigen3"
        "/usr/local/include/eigen3"
    )

    foreach(_candidate IN LISTS _eigen_candidates)
        if(EXISTS "${_candidate}/Eigen/Dense")
            set(${OUTPUT_VAR} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    find_path(_resolved_eigen_include_dir NAMES Eigen/Dense PATH_SUFFIXES eigen3)
    if(_resolved_eigen_include_dir)
        set(${OUTPUT_VAR} "${_resolved_eigen_include_dir}" PARENT_SCOPE)
        return()
    endif()

    FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
    )
    FetchContent_GetProperties(eigen)
    if(NOT eigen_POPULATED)
        FetchContent_Populate(eigen)
    endif()
    set(${OUTPUT_VAR} "${eigen_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

resolve_eigen3_include_dir(EIGEN3_INCLUDE_DIR)

# Shared path variables (must be defined before any target uses them)
set(PACKAGES_ROOT "${COOLBOX_ROOT}/_deliverables/libraries/groups")
set(METADATA_HEADERS_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/MISC/metadata_management/headers")
set(MATRIX_HEADERS_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/DATASTRUCTURE/matrix/headers")
set(MYTRIX_HEADERS_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/MATRIX/headers")
set(RANK_CONVERGENCE_HEADERS_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/ALGORITHM/dynamic_programming/rank_convergence_ltdp")

# Option to control MODULARIZE
option(EMSCRIPTEN_MODULARIZE "Build with MODULARIZE=1" ON)

# If you want to add mytrix, use a separate FetchContent_Declare block:
# FetchContent_Declare(
#     mytrix
#     GIT_REPOSITORY https://example.com/mytrix.git
#     GIT_TAG        master
#     GIT_SHALLOW    TRUE
# )
# set(MYTRIX_BUILD_DOC OFF CACHE BOOL "" FORCE)

set(ADV_LOG_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/IO/advanced_logging")

if(EMSCRIPTEN)
    add_executable(advanced_logging_js
        advanced_logging_bindings.cpp
        ${ADV_LOG_SRC_DIR}/source/advanced_logging.cpp
    )

    target_include_directories(advanced_logging_js PRIVATE
        ${COOLBOX_ROOT}/_deliverables/libraries
        ${ADV_LOG_SRC_DIR}/headers
    )

    # Emscripten-specific flags
    # Example: link to mytrix if needed
    # target_link_libraries(advanced_logging_js PRIVATE mytrix)
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
endif()
# Note: Not building with Emscripten — JS output will not be produced.

 # ── circuitry bindings ──────────────────────────────────────────────
# NOTE: circuitry_js is disabled — circuitry library headers (component.h)
# contain unterminated comments and are not yet compilable with emscripten.
# Re-enable once the circuitry library headers are fixed.
#
# if(EMSCRIPTEN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/circuitry_bindings.cpp")
#     ...
# endif()

# ── battery bindings ────────────────────────────────────────────────
set(BATTERY_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/sim_group/ELECTRONICS/battery")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/battery_bindings.cpp")

    add_executable(battery_js
        battery_bindings.cpp
    )
    target_include_directories(battery_js PRIVATE
        ${COOLBOX_ROOT}/_deliverables/libraries
        ${BATTERY_SRC_DIR}/include
        ${COOLBOX_ROOT}/_deliverables/libraries/groups/sim_group/CHEMISTRY/include
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
set(DS_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/DATASTRUCTURE")

if(EMSCRIPTEN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data_structures_bindings.cpp")
    add_executable(data_structures_js
        data_structures_bindings.cpp
        ${DS_SRC_DIR}/list/source/linked_list.cpp
        ${DS_SRC_DIR}/maps/source/hash_map.cpp
        ${DS_SRC_DIR}/trees/source/binary_search_tree.cpp
    )
    target_include_directories(data_structures_js PRIVATE
        ${DS_SRC_DIR}/headers
        ${DS_SRC_DIR}/trees/headers
        ${DS_SRC_DIR}/maps/headers
        ${DS_SRC_DIR}/list/headers
        ${DS_SRC_DIR}/filters/headers
    )
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
# ── gabor_patches bindings ──────────────────────────────────────────
# NOTE: gabor_patches_js is disabled — gabor_patches.cpp uses matrix::DenseMatrix<Scalar>
# (a template API) but the current MATRIX headers provide mytrix::DenseMatrix (non-template).
# Re-enable once gabor_patches library is updated to use the mytrix API.
set(GP_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/ML/gabor_patches")

# Global include for lib_metadata.h and other shared headers
# (PACKAGES_ROOT, METADATA_HEADERS_DIR, MATRIX_HEADERS_DIR, MYTRIX_HEADERS_DIR
#  are declared at the top of this file so all targets — including manually
#  declared ones like gabor_patches_js — see them.)

# ===================================================================
# Helper macro: add an emscripten JS module from a bindings file + sources
# ===================================================================
macro(add_emscripten_module NAME EXPORT_NAME BINDING_FILE SOURCES INCLUDE_DIRS)
    if(EMSCRIPTEN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BINDING_FILE}")
        add_executable(${NAME} ${BINDING_FILE} ${SOURCES})
        target_include_directories(${NAME} PRIVATE ${INCLUDE_DIRS} ${METADATA_HEADERS_DIR} ${MATRIX_HEADERS_DIR} ${MYTRIX_HEADERS_DIR} ${PACKAGES_ROOT} ${COOLBOX_ROOT}/_deliverables/libraries ${EIGEN3_INCLUDE_DIR})
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
endmacro()

# ── ML library source directories ──────────────────────────────────
set(ML_ROOT "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/ML")
set(DL_ROOT "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/DL")

# ── Decision Tree ──────────────────────────────────────────────────
add_emscripten_module(decision_tree_js createDecisionTreeModule
    decision_tree_bindings.cpp
    "${ML_ROOT}/decision_tree/source/decision_tree.cpp;${ML_ROOT}/decision_tree/source/random_forest.cpp;${ML_ROOT}/decision_tree/source/boost_tree.cpp;${ML_ROOT}/decision_tree/source/rule_set.cpp"
    "${ML_ROOT}/decision_tree/headers"
)

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
    "${ML_ROOT}/hidden_markov_model/headers;${RANK_CONVERGENCE_HEADERS_DIR}"
)

# ── Distributions ────────────────────────────────────────────────
add_emscripten_module(distribution_js createDistributionModule
    distribution_bindings.cpp
    "${ML_ROOT}/distribution/source/normal_distribution.cpp;${ML_ROOT}/distribution/source/bernoulli_distribution.cpp;${ML_ROOT}/distribution/source/binomial_distribution.cpp;${ML_ROOT}/distribution/source/poisson_distribution.cpp;${ML_ROOT}/distribution/source/exponential_distribution.cpp;${ML_ROOT}/distribution/source/gamma_distribution.cpp;${ML_ROOT}/distribution/source/discrete_distribution.cpp;${ML_ROOT}/distribution/source/util.cpp"
    "${ML_ROOT}/distribution/headers"
)

# ── Generalized Linear Model ────────────────────────────────────
add_emscripten_module(glm_js createGLMModule
    glm_bindings.cpp
    "${ML_ROOT}/generalized_linear_model/source/generalized_linear_model.cpp;${ML_ROOT}/generalized_linear_model/source/linear_regression.cpp"
    "${ML_ROOT}/generalized_linear_model/headers;${ML_ROOT}/distribution/headers"
)

# ── Multi-Arm Bandit ─────────────────────────────────────────────
add_emscripten_module(multi_arm_bandit_js createMultiArmBanditModule
    multi_arm_bandit_bindings.cpp
    "${ML_ROOT}/multi_arm_bandit/source/bandit_arm.cpp;${ML_ROOT}/multi_arm_bandit/source/bandit_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/epsilon_greedy_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/ucb_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/thompson_sampling_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/decaying_epsilon_agent.cpp;${ML_ROOT}/multi_arm_bandit/source/simulation_result.cpp"
    "${ML_ROOT}/multi_arm_bandit/headers"
)

# ── Deep Learning ────────────────────────────────────────────────
# NOTE: deep_learning_js is disabled — DL wrapper/layers source files have
# out-of-line method definitions that don't match their header declarations.
# Re-enable once the DL library (neural_network.cpp train(), tensor.cpp flat_index()) is fixed.
#
# add_emscripten_module(deep_learning_js createDeepLearningModule ...)

# ── Support Vector Machine ───────────────────────────────────────
# NOTE: svm_js is disabled — support_vector_machine.cpp calls .at(i,j) on
# Eigen::MatrixXd objects (which don't have that method) and has ambiguous
# overload resolution with the mytrix DenseMatrix API.
# Re-enable once the SVM library is updated to use Eigen or mytrix consistently.
#
# add_emscripten_module(svm_js createSVMModule ...)

# ── Dimensionality Reduction (PCA) ──────────────────────────────
add_emscripten_module(dimensionality_reduction_js createDimReducModule
    dimensionality_reduction_bindings.cpp
    "${ML_ROOT}/dimensionality_reduction/source/pca.cpp;${ML_ROOT}/dimensionality_reduction/source/svd.cpp;${ML_ROOT}/dimensionality_reduction/source/knn.cpp;${ML_ROOT}/dimensionality_reduction/source/umap.cpp"
    "${ML_ROOT}/dimensionality_reduction/headers"
)

# ── Latent Sentiment Analysis ────────────────────────────────────
add_emscripten_module(latent_sentiment_analysis_js createLSAModule
    latent_sentiment_analysis_bindings.cpp
    "${ML_ROOT}/latent_sentiment_analysis/source/latent_sentiment_analysis.cpp"
    "${ML_ROOT}/latent_sentiment_analysis/headers"
)

# ── Marked Point Process ────────────────────────────────────────
add_emscripten_module(marked_point_process_js createMPPModule
    marked_point_process_bindings.cpp
    "${ML_ROOT}/marked_point_process/source/marked_point_process.cpp;${ML_ROOT}/marked_point_process/source/piecewise_conditional_intensity_model.cpp"
    "${ML_ROOT}/marked_point_process/headers"
)

# ── Tracker (Kalman Filters) ────────────────────────────────────
add_emscripten_module(tracker_js createTrackerModule
    tracker_bindings.cpp
    "${ML_ROOT}/tracker/source/kalman_filter.cpp;${ML_ROOT}/tracker/source/extended_kalman_filter.cpp;${ML_ROOT}/tracker/source/unscented_kalman_filter.cpp;${ML_ROOT}/tracker/source/sequential_monte_carlo.cpp"
    "${ML_ROOT}/tracker/headers"
)

# ── Time Series ─────────────────────────────────────────────────
add_emscripten_module(time_series_js createTimeSeriesModule
    time_series_bindings.cpp
    "${ML_ROOT}/time_series/source/time_series.cpp"
    "${ML_ROOT}/time_series/headers"
)

# ── Fourier Transforms ────────────────────────────────────────────
set(FOURIER_TRANFORMS_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/SP/fourier_tranforms")

add_emscripten_module(fourier_tranforms_js createFourierTransformsModule
    fourier_tranforms_bindings.cpp
    "${FOURIER_TRANFORMS_SRC_DIR}/source/fourier_tranforms.cpp"
    "${FOURIER_TRANFORMS_SRC_DIR}/headers"
)

if(TARGET fourier_tranforms_js)
    set_target_properties(fourier_tranforms_js PROPERTIES OUTPUT_NAME "fourier_tranforms")
endif()

# ── Wave Generator ───────────────────────────────────────────────
set(WAVE_GEN_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/MISC/wave_generator")

add_emscripten_module(wave_generator_js createWaveGeneratorModule
    wave_generator_bindings.cpp
    "${WAVE_GEN_SRC_DIR}/source/wave_generator.cpp"
    "${WAVE_GEN_SRC_DIR}/headers"
)

# ── UUID Generation ───────────────────────────────────────────────
set(UUID_GEN_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/trekker/MISC/uuid_generation")

add_emscripten_module(uuid_generation_js createUuidGenerationModule
    uuid_generation_bindings.cpp
    "${UUID_GEN_SRC_DIR}/source/uuid_generation.cpp"
    "${UUID_GEN_SRC_DIR}/headers"
)

if(BUILD_TESTING)
    add_test(
        NAME emscripten_uuid_bindings_cuid_smoke
        COMMAND ${CMAKE_COMMAND}
            -DUUID_BINDINGS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/uuid_generation_bindings.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_uuid_bindings_cuid.cmake
    )
endif()

# ── Audio Processing ─────────────────────────────────────────────
set(AUDIO_PROC_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/audio_visual_group/VIDEO_ASSETS/audio_processing")

add_emscripten_module(audio_processing_js createAudioProcessingModule
    audio_processing_bindings.cpp
    "${AUDIO_PROC_SRC_DIR}/source/audio_buffer.cpp;${AUDIO_PROC_SRC_DIR}/source/resampler.cpp;${AUDIO_PROC_SRC_DIR}/source/mixer.cpp;${AUDIO_PROC_SRC_DIR}/source/effects.cpp"
    "${AUDIO_PROC_SRC_DIR}/headers"
)

# ── VOIP API Exposure ───────────────────────────────────────────
set(VOIP_API_EXPOSURE_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/audio_visual_group/VOIP/api_exposure")

add_emscripten_module(voip_api_exposure_js createVoipApiExposureModule
    voip_api_exposure_bindings.cpp
    "${VOIP_API_EXPOSURE_SRC_DIR}/source/api_exposure.cpp"
    "${VOIP_API_EXPOSURE_SRC_DIR}/headers"
)

# ── PDE/SPDE (MATH) ─────────────────────────────────────────────
set(MATH_SRC_DIR "${COOLBOX_ROOT}/_deliverables/libraries/groups/cool_car/MATH")

add_emscripten_module(pde_spde_js createPdeSpdeModule
    pde_spde_bindings.cpp
    ""
    "${MATH_SRC_DIR}"
)
