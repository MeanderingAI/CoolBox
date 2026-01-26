add_library(dimensionality_reduction SHARED
    source/knn.cpp
    source/pca.cpp
    source/svd.cpp
    source/umap.cpp
)
target_include_directories(dimensionality_reduction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
