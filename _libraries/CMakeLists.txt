# CMakeLists.txt for _libraries
# Helper: only add_subdirectory if the directory actually exists
macro(add_subdirectory_if_exists dir)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
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
        add_subdirectory_if_exists(backages/TOOLS/lsp)
        add_subdirectory_if_exists(backages/PARSER/javascript)
else()
    message(STATUS "Skipping IO SQL backage (BUILD_IO_SQL=OFF)")
endif()
add_subdirectory_if_exists(backages/IO/dataformats/json)
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
add_subdirectory_if_exists(backages/GRAPHICS/charts)
add_subdirectory_if_exists(backages/GRAPHICS/fonts)
add_subdirectory_if_exists(backages/GRAPHICS/components)
add_subdirectory_if_exists(backages/GRAPHICS/windows)

# Electronics backages
add_subdirectory_if_exists(backages/ELECTRONICS/curcuitry)
add_subdirectory_if_exists(backages/ELECTRONICS/battery)
