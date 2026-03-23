# CMakeLists.txt for _libraries
# Helper: only add_subdirectory if the directory actually exists
macro(add_subdirectory_if_exists dir)
    set(target_src "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    if(EXISTS "${target_src}/CMakeLists.txt")
        # Compute the intended binary dir for this subdirectory
        set(target_bin "${CMAKE_BINARY_DIR}/${dir}")

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
        message(STATUS "Skipping ${dir} (not found)")
    endif()
endmacro()

# IO backages
option(BUILD_IO_SQL "Build IO SQL backage (requires SQLite3)" ON)
add_subdirectory_if_exists(backages/MISC/thread_pool)
add_subdirectory_if_exists(backages/IO/advanced_logging)
add_subdirectory_if_exists(backages/DATASTRUCTURE)
if(BUILD_IO_SQL)
    add_subdirectory_if_exists(backages/PARSER/database)
        add_subdirectory_if_exists(backages/PARSER/programming_language)
        add_subdirectory_if_exists(backages/PARSER/java)
        add_subdirectory_if_exists(backages/PARSER/python)
        add_subdirectory_if_exists(backages/PARSER/rust)
        add_subdirectory_if_exists(backages/PARSER/vhdl)
        add_subdirectory_if_exists(backages/PARSER/matlab)
        add_subdirectory_if_exists(backages/TOOLS/lsp)
        add_subdirectory_if_exists(backages/TOOLS/lsp_java)
        add_subdirectory_if_exists(backages/TOOLS/lsp_python)
        add_subdirectory_if_exists(backages/TOOLS/lsp_rust)
        add_subdirectory_if_exists(backages/TOOLS/lsp_vhdl)
        add_subdirectory_if_exists(backages/TOOLS/lsp_matlab)
        add_subdirectory_if_exists(backages/PARSER/javascript)
else()
    message(STATUS "Skipping IO SQL backage (BUILD_IO_SQL=OFF)")
endif()
add_subdirectory_if_exists(backages/IO/dataformats/json)
add_subdirectory_if_exists(backages/IO/databases)
add_subdirectory_if_exists(backages/IO/distributed)
add_subdirectory_if_exists(backages/IO/file_system)

# Utils/security backages needed by auth/http
add_subdirectory_if_exists(backages/MISC/hash)
add_subdirectory_if_exists(backages/MISC/cryptocurrency)
add_subdirectory_if_exists(backages/SECURITY/auth)

add_subdirectory_if_exists(backages/IO/servlets)
add_subdirectory_if_exists(backages/IO/http_server)
add_subdirectory_if_exists(gen_docs)

# Utils backages (before ML — metadata_management is a dependency)
add_subdirectory_if_exists(backages/MISC/metadata_management)
add_subdirectory_if_exists(backages/MISC/unix_commands)
add_subdirectory_if_exists(backages/MISC/system_stats)
add_subdirectory_if_exists(backages/MISC/wave_generator)

# ML backages
add_subdirectory_if_exists(backages/ML/bayesian_network_ai)
add_subdirectory_if_exists(backages/ML/computer_vision)
add_subdirectory_if_exists(backages/ML/decision_tree)
add_subdirectory_if_exists(backages/ML/deep_learning)
add_subdirectory_if_exists(backages/ML/dimensionality_reduction)
add_subdirectory_if_exists(backages/ML/distribution)
add_subdirectory_if_exists(backages/ML/generalized_linear_model)
add_subdirectory_if_exists(backages/ML/hidden_markov_model)
add_subdirectory_if_exists(backages/ML/latent_sentiment_analysis)
add_subdirectory_if_exists(backages/ML/marked_point_process)
add_subdirectory_if_exists(backages/ML/multi_arm_bandit)
add_subdirectory_if_exists(backages/ML/nlp)
add_subdirectory_if_exists(backages/ML/support_vector_machine)
add_subdirectory_if_exists(backages/ML/time_series)
add_subdirectory_if_exists(backages/ML/tracker)
add_subdirectory_if_exists(backages/ML/self_organizing_maps)
add_subdirectory_if_exists(backages/ML/vae)
add_subdirectory_if_exists(backages/ML/knn)
add_subdirectory_if_exists(backages/ML/gabor_patches)

# Security backages
add_subdirectory_if_exists(backages/SECURITY/fuzzer)
add_subdirectory_if_exists(backages/SECURITY/malware_scanner)
add_subdirectory_if_exists(backages/SECURITY/network_scanner)

# Graphics backages
add_subdirectory_if_exists(backages/GRAPHICS)
add_subdirectory_if_exists(backages/GRAPHICS/charts)
add_subdirectory_if_exists(backages/GRAPHICS/fonts)
add_subdirectory_if_exists(backages/GRAPHICS/components)
add_subdirectory_if_exists(backages/GRAPHICS/windows)

# Chemistry backage (provides periodic_table.h)
add_subdirectory_if_exists(backages/CHEMISTRY)

# Electronics backages
add_subdirectory_if_exists(backages/ELECTRONICS/curcuitry)
add_subdirectory_if_exists(backages/ELECTRONICS/battery)
