cmake_minimum_required(VERSION 3.12)

add_library(candecomp_parafac SHARED
    source/cp_decomposition.cpp
)

target_include_directories(candecomp_parafac PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(candecomp_parafac PUBLIC cxx_std_17)
target_link_libraries(candecomp_parafac PUBLIC mytrix)

if(BUILD_TESTING)
    enable_testing()
    add_executable(candecomp_parafac_tests
        tests/test_cp_decomposition.cpp
        ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
    )
    target_include_directories(candecomp_parafac_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
    )
    target_link_libraries(candecomp_parafac_tests PRIVATE candecomp_parafac)
    coolbox_stage_test_runtime_dependencies(candecomp_parafac_tests)
    add_test(NAME CandecompParafacTests COMMAND candecomp_parafac_tests)
endif()
