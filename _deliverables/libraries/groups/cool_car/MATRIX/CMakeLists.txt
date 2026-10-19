# CMakeLists.txt for MATRIX package (mytrix - header-only Eigen wrapper)

add_library(mytrix INTERFACE)

option(MYTRIX_ENABLE_GPU "Enable GPU backend plumbing in mytrix" ON)
option(MYTRIX_ENABLE_OPENCL "Enable OpenCL backend in mytrix" OFF)
option(MYTRIX_ENABLE_CUDA "Enable CUDA backend in mytrix" OFF)
option(MYTRIX_ENABLE_METAL "Enable Metal backend in mytrix" OFF)

target_include_directories(mytrix INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
    $<INSTALL_INTERFACE:include>
)

find_package(Eigen3 QUIET)
if(TARGET Eigen3::Eigen)
    target_link_libraries(mytrix INTERFACE Eigen3::Eigen)
endif()

target_compile_features(mytrix INTERFACE cxx_std_17)

if(MYTRIX_ENABLE_GPU)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_GPU=1)
endif()

if(MYTRIX_ENABLE_OPENCL)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_OPENCL=1)
    message(WARNING
        "MATRIX OpenCL kernels are not implemented; the OpenCL backend will remain unavailable."
    )
endif()

if(MYTRIX_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "MYTRIX_ENABLE_CUDA requires an nvcc-compatible CUDA compiler.")
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_CUDA=1)
    target_link_libraries(mytrix INTERFACE CUDA::cudart)
endif()

if(MYTRIX_ENABLE_METAL)
    if(NOT APPLE)
        message(FATAL_ERROR "MYTRIX_ENABLE_METAL is only supported on Apple platforms.")
    endif()
    enable_language(OBJCXX)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_METAL=1)
    target_link_libraries(mytrix INTERFACE
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()

if(MYTRIX_ENABLE_METAL AND APPLE)
    target_compile_definitions(mytrix INTERFACE
        MYTRIX_METAL_SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/source/metal_backend.metal"
    )
    target_sources(mytrix INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/source/metal_backend.metal
        ${CMAKE_CURRENT_SOURCE_DIR}/source/metal_backend.mm
    )
endif()

if(BUILD_TESTING)
    enable_testing()
    add_executable(mytrix_cpu_optimizations_tests
        tests/test_cpu_optimizations.cpp
    )
    target_link_libraries(mytrix_cpu_optimizations_tests PRIVATE mytrix)
    add_test(NAME MytrixCpuOptimizationsTests COMMAND mytrix_cpu_optimizations_tests)

    add_executable(mytrix_backend_selection_tests
        tests/test_backend_selection.cpp
    )
    if(MYTRIX_ENABLE_CUDA)
        set_source_files_properties(tests/test_backend_selection.cpp PROPERTIES LANGUAGE CUDA)
    elseif(MYTRIX_ENABLE_METAL)
        set_source_files_properties(tests/test_backend_selection.cpp PROPERTIES LANGUAGE OBJCXX)
    endif()
    target_link_libraries(mytrix_backend_selection_tests PRIVATE mytrix)
    add_test(NAME MytrixBackendSelectionTests COMMAND mytrix_backend_selection_tests)
endif()
