# CMakeLists.txt for _libraries
# Helper: only add_subdirectory if the directory actually exists
macro(add_subdirectory_if_exists dir)
    set(target_src "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    # Allow CI to require that all backages exist. When FORCE_ADD_ALL_BACKAGES=ON
    # the build will fail if a listed backage directory is missing. This helps
    # CI detect missing checkouts or path issues instead of silently skipping
    # targets.
    if(NOT DEFINED FORCE_ADD_ALL_BACKAGES)
        option(FORCE_ADD_ALL_BACKAGES "Fail if backage directories are missing (CI use)" OFF)
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
        if(FORCE_ADD_ALL_BACKAGES)
            message(FATAL_ERROR "Required backage directory not found: ${target_src}")
        else()
            message(STATUS "Skipping ${dir} (not found)")
        endif()
    endif()
endmacro()

# Force-add a subdirectory: compute a stable binary dir and require the
# source directory to exist. Use in CI/configured builds that must include
# every backage; this avoids silently skipping missing backages.
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
        message(FATAL_ERROR "Required backage directory not found: ${target_src}")
    endif()
endmacro()

# IO backages
option(BUILD_IO_SQL "Build IO SQL backage (requires SQLite3)" ON)
add_subdirectory_if_exists(backages/MISC/thread_pool)
add_subdirectory_if_exists(backages/IO/advanced_logging)
add_subdirectory_if_exists(backages/DATASTRUCTURE)  # Now includes matrix library
if(BUILD_PRODUCT_INSTALLER_ABSTRACTIONS)
    add_subdirectory_if_exists(backages/OS_GENERICS)
endif()
if(BUILD_IO_SQL)
    add_subdirectory_if_exists(backages/PARSER/database)
        add_subdirectory_if_exists(backages/PARSER/programming_language)
        # Parser libraries: names confirmed by CMakeLists.txt in each folder
        add_subdirectory_if_exists(backages/PARSER/java)      # pljava_parser_lib
        add_subdirectory_if_exists(backages/PARSER/python)    # plpython_parser_lib
        add_subdirectory_if_exists(backages/PARSER/rust)      # plrust_parser_lib
        add_subdirectory_if_exists(backages/PARSER/vhdl)
        add_subdirectory_if_exists(backages/PARSER/matlab)
        add_subdirectory_if_exists(backages/TOOLS/lsp)
        # LSP libraries: names confirmed by CMakeLists.txt in each folder
        add_subdirectory_if_exists(backages/TOOLS/lsp_java)      # pljava_lsp_lib
        add_subdirectory_if_exists(backages/TOOLS/lsp_python)    # plpython_lsp_lib
        add_subdirectory_if_exists(backages/TOOLS/lsp_rust)      # plrust_lsp_lib
        add_subdirectory_if_exists(backages/TOOLS/lsp_vhdl)      # plvhdl_lsp_lib
        add_subdirectory_if_exists(backages/TOOLS/lsp_matlab)    # plmatlab_lsp_lib
        add_subdirectory_if_exists(backages/PARSER/javascript)
else()
    message(STATUS "Skipping IO SQL backage (BUILD_IO_SQL=OFF)")
endif()
    add_subdirectory_if_exists(backages/TOOLS/tyst_framework)
    add_subdirectory_if_exists(backages/TOOLS/bower_shell)
    add_subdirectory_if_exists(backages/TOOLS/file_browser)
add_subdirectory_if_exists(backages/IO/dataformats/json)  # json library now located here
add_subdirectory_if_exists(backages/IO/databases)
add_subdirectory_if_exists(backages/IO/distributed)
add_subdirectory_if_exists(backages/IO/file_system)
add_subdirectory_if_exists(backages/IO/quic_transport)

# Utils/security backages needed by auth/http
add_subdirectory_force(backages/MISC/hash)
add_subdirectory_force(backages/MISC/cryptocurrency)
add_subdirectory_force(backages/SECURITY/auth)

add_subdirectory_force(backages/IO/servlets)
add_subdirectory_force(backages/IO/http_server)
if(EXISTS "${TOOLBOX_SOURCE_DIR}/_libraries/include")
    add_subdirectory_force(gen_docs)
endif()

# Utils backages (before ML — metadata_management is a dependency)
add_subdirectory_force(backages/MISC/metadata_management)
add_subdirectory_force(backages/MISC/unix_commands)
add_subdirectory_force(backages/MISC/system_stats)
add_subdirectory_force(backages/MISC/wave_generator)

# ML backages
add_subdirectory_force(backages/ML/bayesian_network_ai)
add_subdirectory_force(backages/ML/computer_vision)
add_subdirectory_force(backages/ML/decision_tree)
add_subdirectory_force(backages/ML/deep_learning)
add_subdirectory_force(backages/ML/dimensionality_reduction)
add_subdirectory_force(backages/ML/distribution)
add_subdirectory_force(backages/ML/generalized_linear_model)
add_subdirectory_force(backages/ML/hidden_markov_model)
add_subdirectory_force(backages/ML/latent_sentiment_analysis)
add_subdirectory_force(backages/ML/marked_point_process)
add_subdirectory_force(backages/ML/multi_arm_bandit)
add_subdirectory_force(backages/ML/nlp)
add_subdirectory_force(backages/ML/support_vector_machine)
add_subdirectory_force(backages/ML/time_series)
add_subdirectory_force(backages/ML/tracker)
add_subdirectory_force(backages/ML/self_organizing_maps)
add_subdirectory_force(backages/ML/vae)
add_subdirectory_force(backages/ML/knn)
add_subdirectory_force(backages/ML/gabor_patches)

# Security backages
add_subdirectory_force(backages/SECURITY/fuzzer)
add_subdirectory_force(backages/SECURITY/malware_scanner)
add_subdirectory_force(backages/SECURITY/network_scanner)

# Graphics backages
#add_subdirectory_force(backages/GRAPHICS)
add_subdirectory_force(backages/GRAPHICS/charts)
add_subdirectory_force(backages/GRAPHICS/fonts)
add_subdirectory_force(backages/GRAPHICS/components)
add_subdirectory_force(backages/GRAPHICS/windows)
add_subdirectory_force(backages/GRAPHICS/full_application_window)

# Chemistry backage (provides periodic_table.h)
add_subdirectory_force(backages/CHEMISTRY)

# Electronics backages
add_subdirectory_force(backages/ELECTRONICS/curcuitry)
add_subdirectory_force(backages/ELECTRONICS/battery)

# Signal processing backages
add_subdirectory_force(backages/SP)
