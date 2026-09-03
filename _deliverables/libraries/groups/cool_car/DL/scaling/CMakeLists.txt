# CMakeLists.txt for DL/scaling
#
# Large-scale training and inference techniques from the DeepSpeed papers.
# See include/scaling_citations.h and
# _internal_documents/engineering_documention/bib/scaling_papers.bib.

cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

set(DL_SCALING_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DL_GROUP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(dl_scaling STATIC
    src/collective.cpp
    src/domino.cpp
    src/fast_persist.cpp
    src/moe.cpp
    src/one_bit_lamb.cpp
    src/scaling_citations.cpp
    src/universal_checkpoint.cpp
    src/zero.cpp
    src/zero_infinity.cpp
    src/zero_quant.cpp
    ${DL_GROUP_ROOT}/layers/src/tensor.cpp
)

target_include_directories(dl_scaling PUBLIC
    ${DL_SCALING_ROOT}/include
    ${DL_GROUP_ROOT}/layers/include
    ${DL_GROUP_ROOT}/optimizer/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers
)

target_compile_features(dl_scaling PUBLIC cxx_std_17)
target_link_libraries(dl_scaling PUBLIC Threads::Threads)

find_package(Eigen3 QUIET)
if(TARGET Eigen3::Eigen)
    target_link_libraries(dl_scaling PUBLIC Eigen3::Eigen)
endif()

if(BUILD_TESTING)
    enable_testing()
    # The shared tyst target is not always in the configure graph; fall back to
    # the framework main source when it is missing.
    if(TARGET tyst_framework_main)
        add_executable(dl_scaling_tests tests/test_scaling.cpp)
        target_link_libraries(dl_scaling_tests PRIVATE dl_scaling tyst_framework_main)
    else()
        add_executable(dl_scaling_tests
            tests/test_scaling.cpp
            ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
        )
        target_include_directories(dl_scaling_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
        )
        target_link_libraries(dl_scaling_tests PRIVATE dl_scaling)
    endif()

    coolbox_stage_test_runtime_dependencies(dl_scaling_tests)
    add_test(NAME DeepLearningScalingTests COMMAND dl_scaling_tests)
endif()
