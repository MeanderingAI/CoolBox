# CMakeLists.txt for _libraries
# Add subdirectories for all relevant library components
add_subdirectory(backages/IO/advanced_logging)
add_subdirectory(backages/IO/data_structures)
add_subdirectory(backages/IO/databases/sql)
add_subdirectory(backages/IO/dataformats/json)
add_subdirectory(backages/IO/distributed)
add_subdirectory(backages/IO/file_system)
add_subdirectory(backages/IO/http_server)
add_subdirectory(backages/IO/servlets)
add_subdirectory(gen_docs)

# ML backages
add_subdirectory(backages/ML/bayesian_network)
add_subdirectory(backages/ML/computer_vision)
add_subdirectory(backages/ML/decision_tree)
add_subdirectory(backages/ML/deep_learning)
add_subdirectory(backages/ML/dimensionality_reduction)
add_subdirectory(backages/ML/distribution)
add_subdirectory(backages/ML/generalized_linear_model)
add_subdirectory(backages/ML/hidden_markov_model)
add_subdirectory(backages/ML/latent_sentiment_analysis)
add_subdirectory(backages/ML/marked_point_process)
add_subdirectory(backages/ML/multi_arm_bandit)
add_subdirectory(backages/ML/nlp)
add_subdirectory(backages/ML/support_vector_machine)
add_subdirectory(backages/ML/time_series)
add_subdirectory(backages/ML/tracker)
# add_subdirectory(backages/ML/knn)  # incomplete: missing source/knn.cpp

# Security backages
add_subdirectory(backages/security/auth)
add_subdirectory(backages/security/fuzzer)
add_subdirectory(backages/security/malware_scanner)
add_subdirectory(backages/security/network_scanner)

# Utils backages

add_subdirectory(backages/MISC/unix_commands)
add_subdirectory(backages/MISC/system_stats)

# Electronics backages
add_subdirectory(backages/ELECTRONICS/curcuitry)
add_subdirectory(backages/ELECTRONICS/battery)
