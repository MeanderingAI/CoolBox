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
add_subdirectory_if_exists(backages/IO/advanced_logging)
add_subdirectory_if_exists(backages/IO/data_structures)
add_subdirectory_if_exists(backages/IO/databases/sql)
add_subdirectory_if_exists(backages/IO/dataformats/json)
add_subdirectory_if_exists(backages/IO/distributed)
add_subdirectory_if_exists(backages/IO/file_system)
add_subdirectory_if_exists(backages/IO/http_server)
add_subdirectory_if_exists(backages/IO/servlets)
add_subdirectory_if_exists(gen_docs)

# ML backages
add_subdirectory_if_exists(backages/ML/bayesian_network)
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
# add_subdirectory_if_exists(backages/ML/knn)  # incomplete: missing source/knn.cpp

# Security backages
add_subdirectory_if_exists(backages/security/auth)
add_subdirectory_if_exists(backages/security/fuzzer)
add_subdirectory_if_exists(backages/security/malware_scanner)
add_subdirectory_if_exists(backages/security/network_scanner)

# Utils backages
add_subdirectory_if_exists(backages/MISC/unix_commands)
add_subdirectory_if_exists(backages/MISC/system_stats)

# Electronics backages
add_subdirectory_if_exists(backages/ELECTRONICS/curcuitry)
add_subdirectory_if_exists(backages/ELECTRONICS/battery)
