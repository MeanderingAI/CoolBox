# knn is header-only (source/knn.cpp does not exist yet)
add_library(knn INTERFACE)
target_include_directories(knn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
