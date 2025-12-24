cmake_minimum_required(VERSION 3.12)

# Find Eigen3
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# Create dimensionality reduction library
add_library(dimensionality_reduction SHARED
    svd.cpp
    pca.cpp
    knn.cpp
    umap.cpp
)

target_include_directories(dimensionality_reduction PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(dimensionality_reduction
    Eigen3::Eigen
)

set_target_properties(dimensionality_reduction PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Install library
install(TARGETS dimensionality_reduction
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
