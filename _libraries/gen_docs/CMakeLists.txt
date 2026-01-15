# Debug: print TOOLBOX_SOURCE_DIR value
message(STATUS "TOOLBOX_SOURCE_DIR is: ${TOOLBOX_SOURCE_DIR}")
# CMakeLists.txt for gen_docs
cmake_minimum_required(VERSION 3.10)
project(gen_docs)


# Use parent project's Doxygen (from ExternalDependencies.cmake)
if(NOT DEFINED DOXYGEN_EXECUTABLE)
    find_package(Doxygen QUIET)
endif()
if(NOT DOXYGEN_EXECUTABLE)
    message(FATAL_ERROR "Doxygen not found. Please ensure it is installed or fetched by the top-level CMake.")
endif()



# Ensure TOOLBOX_SOURCE_DIR is set for configure_file
if(NOT DEFINED TOOLBOX_SOURCE_DIR)
    set(TOOLBOX_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
endif()


set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)


# Per-library documentation


# Per-library documentation (one HTML folder per leaf library directory)
file(GLOB_RECURSE LEAF_LIB_DIRS LIST_DIRECTORIES true RELATIVE "${TOOLBOX_SOURCE_DIR}/_libraries/include" "${TOOLBOX_SOURCE_DIR}/_libraries/include/*")
foreach(libdir ${LEAF_LIB_DIRS})
    if(IS_DIRECTORY "${TOOLBOX_SOURCE_DIR}/_libraries/include/${libdir}")
        # Check if this directory contains at least one .h file
        file(GLOB HEADER_FILES "${TOOLBOX_SOURCE_DIR}/_libraries/include/${libdir}/*.h")
        if(HEADER_FILES)
            string(REPLACE "/" "_" libid ${libdir})
            set(DOC_OUT "${TOOLBOX_SOURCE_DIR}/gen_docs/html/libs/${libdir}")
            file(MAKE_DIRECTORY ${DOC_OUT})
            set(DOXYFILE_LIB ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_${libid})
            configure_file(${DOXYGEN_IN} ${DOXYFILE_LIB} @ONLY)
            file(WRITE ${DOXYFILE_LIB} 
                "PROJECT_NAME = \"${libdir} API Documentation\"\n"
            )
            file(APPEND ${DOXYFILE_LIB} "OUTPUT_DIRECTORY = ${DOC_OUT}\n")
            file(APPEND ${DOXYFILE_LIB} "INPUT = ${TOOLBOX_SOURCE_DIR}/_libraries/include/${libdir}\n")
            file(APPEND ${DOXYFILE_LIB} "RECURSIVE = YES\n")
            file(APPEND ${DOXYFILE_LIB} "GENERATE_HTML = YES\n")
            file(APPEND ${DOXYFILE_LIB} "GENERATE_LATEX = NO\n")
            file(APPEND ${DOXYFILE_LIB} "EXTRACT_ALL = YES\n")
            file(APPEND ${DOXYFILE_LIB} "QUIET = YES\n")
            file(APPEND ${DOXYFILE_LIB} "FILE_PATTERNS = *.h *.hpp\n")
            add_custom_command(
                OUTPUT ${DOC_OUT}/index.html
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_LIB}
                DEPENDS ${HEADER_FILES}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating docs for ${libdir}"
            )
            add_custom_target(doc_${libid} ALL DEPENDS ${DOC_OUT}/index.html)
        endif()
    endif()
endforeach()

# Monolithic docs (all headers)
configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html)
add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
