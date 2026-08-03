add_library(distributed_learning SHARED
    source/distributed_owa.cpp
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/layers/src/tensor.cpp
)
target_include_directories(distributed_learning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(distributed_learning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)
target_include_directories(distributed_learning PUBLIC ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/layers/include)
target_include_directories(distributed_learning PUBLIC ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers)
find_package(Eigen3 QUIET)
if(TARGET Eigen3::Eigen)
    target_link_libraries(distributed_learning PUBLIC Eigen3::Eigen)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(distributed_learning_tests
        tests/test_distributed_owa.cpp
    )
    target_include_directories(distributed_learning_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../headers
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/layers/include
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers
    )
    target_link_libraries(distributed_learning_tests PRIVATE distributed_learning tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(distributed_learning_tests)
    add_test(NAME DistributedLearningTests COMMAND distributed_learning_tests)
endif()