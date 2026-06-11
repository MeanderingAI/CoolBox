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
endif()

if(MYTRIX_ENABLE_CUDA)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_CUDA=1)
endif()

if(MYTRIX_ENABLE_METAL)
    target_compile_definitions(mytrix INTERFACE MYTRIX_ENABLE_METAL=1)
endif()

