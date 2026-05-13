# CMakeLists.txt for MATRIX package (mytrix - header-only Eigen wrapper)

add_library(mytrix INTERFACE)

target_include_directories(mytrix INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
    $<INSTALL_INTERFACE:include>
)

find_package(Eigen3 QUIET)
if(TARGET Eigen3::Eigen)
    target_link_libraries(mytrix INTERFACE Eigen3::Eigen)
endif()

target_compile_features(mytrix INTERFACE cxx_std_17)

