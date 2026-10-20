include_guard(GLOBAL)
option(MYTRIX_ENABLE_OPENBLAS "Enable optional OpenBLAS matrix multiplication" OFF)

function(mytrix_link_openblas target visibility)
    if(NOT MYTRIX_ENABLE_OPENBLAS)
        return()
    endif()
    find_path(MYTRIX_OPENBLAS_INCLUDE_DIR cblas.h
        HINTS ${OpenBLAS_ROOT} ENV OpenBLAS_ROOT
        PATH_SUFFIXES include include/openblas openblas)
    find_library(MYTRIX_OPENBLAS_LIBRARY NAMES openblas
        HINTS ${OpenBLAS_ROOT} ENV OpenBLAS_ROOT PATH_SUFFIXES lib lib64)
    if(NOT MYTRIX_OPENBLAS_INCLUDE_DIR OR NOT MYTRIX_OPENBLAS_LIBRARY)
        message(FATAL_ERROR "OpenBLAS requested but not found. Install OpenBLAS development files or set MYTRIX_OPENBLAS_INCLUDE_DIR and MYTRIX_OPENBLAS_LIBRARY.")
    endif()
    target_include_directories(${target} ${visibility} ${MYTRIX_OPENBLAS_INCLUDE_DIR})
    target_link_libraries(${target} ${visibility} ${MYTRIX_OPENBLAS_LIBRARY})
    target_compile_definitions(${target} ${visibility} MYTRIX_ENABLE_OPENBLAS=1)
endfunction()
