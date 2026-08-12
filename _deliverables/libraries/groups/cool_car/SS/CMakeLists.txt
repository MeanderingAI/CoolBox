add_library(SS SHARED
    source/smt_sat_solver.cpp
)

target_include_directories(SS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../OPT/headers
)
target_link_libraries(SS PUBLIC mytrix)
target_compile_features(SS PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(smt_sat_solver_tests
        tests/test_smt_sat_solver.cpp
    )
    target_link_libraries(smt_sat_solver_tests PRIVATE SS hill_climbing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(smt_sat_solver_tests)
    add_test(NAME SmtSatSolverTests COMMAND smt_sat_solver_tests)
endif()