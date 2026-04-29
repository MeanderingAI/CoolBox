# CMakeLists.txt for _libraries
# Helper: only add_subdirectory if the directory actually exists
macro(add_subdirectory_if_exists dir)
    set(target_src "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    # Allow CI to require that all packages exist. When FORCE_ADD_ALL_PACKAGES=ON
    # the build will fail if a listed package directory is missing. This helps
    # CI detect missing checkouts or path issues instead of silently skipping
    # targets.
    if(NOT DEFINED FORCE_ADD_ALL_PACKAGES)
        option(FORCE_ADD_ALL_PACKAGES "Fail if package directories are missing (CI use)" OFF)
    endif()
    if(EXISTS "${target_src}/CMakeLists.txt")
        # Compute the intended binary dir for this subdirectory.
        # Use a path that includes the current source directory relative
        # to the top-level source tree so similarly-named subfolders
        # under different parents do not conflict.
        file(RELATIVE_PATH _rel_src "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        if("${_rel_src}" STREQUAL "")
            set(_rel_src ".")
        endif()
        set(target_bin "${CMAKE_BINARY_DIR}/${_rel_src}/${dir}")

        # If the binary dir already contains a previous CMake configure,
        # check whether it was configured for the same source directory.
        if(EXISTS "${target_bin}/CMakeCache.txt")
            file(READ "${target_bin}/CMakeCache.txt" _cmake_cache_contents)
            string(REGEX MATCH "CMAKE_HOME_DIRECTORY:INTERNAL=([^\n\r]+)" _match "${_cmake_cache_contents}")
            if(_match)
                string(REGEX REPLACE "CMAKE_HOME_DIRECTORY:INTERNAL=([^\n\r]+)" "\\1" cache_home "${_match}")
            else()
                string(REGEX MATCH "CMAKE_SOURCE_DIR:INTERNAL=([^\n\r]+)" _match2 "${_cmake_cache_contents}")
                if(_match2)
                    string(REGEX REPLACE "CMAKE_SOURCE_DIR:INTERNAL=([^\n\r]+)" "\\1" cache_home "${_match2}")
                endif()
            endif()

            if(DEFINED cache_home AND NOT cache_home STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
                message(FATAL_ERROR "The binary directory\n  ${target_bin}\nwas already used to build source directory\n  ${cache_home}\nIt cannot be used to build\n  ${target_src}\n\nPlease remove the conflicting build directory or configure this project using a different binary directory.")
            endif()
        endif()

        add_subdirectory(${dir} ${target_bin})
    else()
        if(FORCE_ADD_ALL_PACKAGES)
            message(FATAL_ERROR "Required package directory not found: ${target_src}")
        else()
            message(STATUS "Skipping ${dir} (not found)")
        endif()
    endif()
endmacro()

# Force-add a subdirectory: compute a stable binary dir and require the
# source directory to exist. Use in CI/configured builds that must include
# every package; this avoids silently skipping missing packages.
macro(add_subdirectory_force dir)
    set(target_src "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    if(EXISTS "${target_src}/CMakeLists.txt")
        file(RELATIVE_PATH _rel_src "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        if("${_rel_src}" STREQUAL "")
            set(_rel_src ".")
        endif()
        set(target_bin "${CMAKE_BINARY_DIR}/${_rel_src}/${dir}")

        # If the binary dir already contains a previous CMake configure,
        # check whether it was configured for the same source directory.
        if(EXISTS "${target_bin}/CMakeCache.txt")
            file(READ "${target_bin}/CMakeCache.txt" _cmake_cache_contents)
            string(REGEX MATCH "CMAKE_HOME_DIRECTORY:INTERNAL=([^\n\r]+)" _match "${_cmake_cache_contents}")
            if(_match)
                string(REGEX REPLACE "CMAKE_HOME_DIRECTORY:INTERNAL=([^\n\r]+)" "\\1" cache_home "${_match}")
            else()
                string(REGEX MATCH "CMAKE_SOURCE_DIR:INTERNAL=([^\n\r]+)" _match2 "${_cmake_cache_contents}")
                if(_match2)
                    string(REGEX REPLACE "CMAKE_SOURCE_DIR:INTERNAL=([^\n\r]+)" "\\1" cache_home "${_match2}")
                endif()
            endif()

            if(DEFINED cache_home AND NOT cache_home STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
                message(FATAL_ERROR "The binary directory\n  ${target_bin}\nwas already used to build source directory\n  ${cache_home}\nIt cannot be used to build\n  ${target_src}\n\nPlease remove the conflicting build directory or configure this project using a different binary directory.")
            endif()
        endif()

        add_subdirectory(${dir} ${target_bin})
    else()
        message(FATAL_ERROR "Required package directory not found: ${target_src}")
    endif()
endmacro()

# IO packages
option(BUILD_IO_SQL "Build IO SQL package (requires SQLite3)" ON)
add_subdirectory_if_exists(packages/MISC/thread_pool)
add_subdirectory_if_exists(packages/IO/advanced_logging)
add_subdirectory_if_exists(packages/DATASTRUCTURE)  # Now includes matrix library
if(BUILD_PRODUCT_INSTALLER_ABSTRACTIONS)
    add_subdirectory_if_exists(packages/OS_GENERICS)
endif()
if(BUILD_IO_SQL)
    add_subdirectory_if_exists(packages/PARSER/database)
        add_subdirectory_if_exists(packages/PARSER/programming_language)
        # Parser libraries: names confirmed by CMakeLists.txt in each folder
        add_subdirectory_if_exists(packages/PARSER/java)      # pljava_parser_lib
        add_subdirectory_if_exists(packages/PARSER/python)    # plpython_parser_lib
        add_subdirectory_if_exists(packages/PARSER/rust)      # plrust_parser_lib
        add_subdirectory_if_exists(packages/PARSER/vhdl)
        add_subdirectory_if_exists(packages/PARSER/matlab)
        add_subdirectory_if_exists(packages/LSP/lsp)
        # LSP libraries: names confirmed by CMakeLists.txt in each folder
        add_subdirectory_if_exists(packages/LSP/lsp_java)      # pljava_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_python)    # plpython_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_rust)      # plrust_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_vlang)     # plvlang_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_c3)        # plc3_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_vhdl)      # plvhdl_lsp_lib
        add_subdirectory_if_exists(packages/LSP/lsp_matlab)    # plmatlab_lsp_lib
        add_subdirectory_if_exists(packages/PARSER/javascript)
else()
    message(STATUS "Skipping IO SQL package (BUILD_IO_SQL=OFF)")
endif()
    add_subdirectory_if_exists(packages/TOOLS/tyst_framework)
    add_subdirectory_if_exists(packages/TOOLS/bower_shell)
    add_subdirectory_if_exists(packages/TOOLS/file_browser)
add_subdirectory_if_exists(packages/IO/dataformats/json)  # json library now located here
add_subdirectory_if_exists(packages/IO/databases)
add_subdirectory_if_exists(packages/IO/distributed)
add_subdirectory_if_exists(packages/IO/file_system)
add_subdirectory_if_exists(packages/IO/quic_transport)
add_subdirectory_if_exists(packages/DL)

# Utils/security packages needed by auth/http
add_subdirectory_force(packages/MISC/hash)
add_subdirectory_force(packages/MISC/cryptocurrency)
add_subdirectory_force(packages/SECURITY/auth)

add_subdirectory_force(packages/IO/servlets)
add_subdirectory_force(packages/IO/http_server)
if(EXISTS "${TOOLBOX_SOURCE_DIR}/_libraries/include")
    add_subdirectory_force(gen_docs)
endif()

# Utils packages (before ML — metadata_management is a dependency)
add_subdirectory_force(packages/MISC/metadata_management)
add_subdirectory_force(packages/MISC/unix_commands)
add_subdirectory_force(packages/MISC/system_stats)
add_subdirectory_force(packages/MISC/wave_generator)

# ML packages
add_subdirectory_force(packages/ML/bayesian_network_ai)
add_subdirectory_force(packages/ML/computer_vision)
add_subdirectory_force(packages/ML/decision_tree)
add_subdirectory_force(packages/ML/dimensionality_reduction)
add_subdirectory_force(packages/ML/distribution)
add_subdirectory_force(packages/ML/generalized_linear_model)
add_subdirectory_force(packages/ML/hidden_markov_model)
add_subdirectory_force(packages/ML/latent_sentiment_analysis)
add_subdirectory_force(packages/ML/marked_point_process)
add_subdirectory_force(packages/ML/multi_arm_bandit)
add_subdirectory_force(packages/ML/nlp)
add_subdirectory_force(packages/ML/support_vector_machine)
add_subdirectory_force(packages/ML/time_series)
add_subdirectory_force(packages/ML/tracker)
add_subdirectory_force(packages/ML/self_organizing_maps)
add_subdirectory_force(packages/ML/vae)
add_subdirectory_force(packages/ML/knn)
add_subdirectory_force(packages/ML/gabor_patches)

# Security packages
add_subdirectory_force(packages/SECURITY/fuzzer)
add_subdirectory_force(packages/SECURITY/malware_scanner)
add_subdirectory_force(packages/SECURITY/network_scanner)

# Graphics packages
#add_subdirectory_force(packages/GRAPHICS)
add_subdirectory_force(packages/GRAPHICS/charts)
add_subdirectory_force(packages/GRAPHICS/fonts)
add_subdirectory_force(packages/GRAPHICS/components)
add_subdirectory_force(packages/GRAPHICS/windows)
add_subdirectory_force(packages/GRAPHICS/full_application_window)

# Chemistry package (provides periodic_table.h)
add_subdirectory_force(packages/CHEMISTRY)

# Electronics packages

add_subdirectory_force(packages/ELECTRONICS/battery)

# Signal processing packages
add_subdirectory_force(packages/SP)
