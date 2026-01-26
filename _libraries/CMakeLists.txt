# CMakeLists.txt for _libraries
# Add subdirectories for all relevant library components
add_subdirectory(packages/IO/advanced_logging)
add_subdirectory(packages/IO/data_structures)
add_subdirectory(packages/IO/databases/sql)
add_subdirectory(packages/IO/dataformats/json)
add_subdirectory(packages/IO/distributed)
add_subdirectory(packages/IO/file_system)
add_subdirectory(packages/IO/http_server)
add_subdirectory(packages/IO/servlets)
add_subdirectory(packages/utils/thread_pool)
add_subdirectory(gen_docs)

# ML packages
add_subdirectory(packages/ML/bayesian_network)
add_subdirectory(packages/ML/computer_vision)
add_subdirectory(packages/ML/decision_tree)
add_subdirectory(packages/ML/deep_learning)
add_subdirectory(packages/ML/dimensionality_reduction)
add_subdirectory(packages/ML/distribution)
add_subdirectory(packages/ML/generalized_linear_model)
add_subdirectory(packages/ML/hidden_markov_model)
add_subdirectory(packages/ML/latent_sentiment_analysis)
add_subdirectory(packages/ML/marked_point_process)
add_subdirectory(packages/ML/multi_arm_bandit)
add_subdirectory(packages/ML/nlp)
add_subdirectory(packages/ML/support_vector_machine)
add_subdirectory(packages/ML/time_series)
add_subdirectory(packages/ML/tracker)

# Security packages
add_subdirectory(packages/security/auth)
add_subdirectory(packages/security/fuzzer)
add_subdirectory(packages/security/malware_scanner)
add_subdirectory(packages/security/network_scanner)

# Utils packages
add_subdirectory(packages/utils/system_stats)
add_subdirectory(packages/utils/unix_commands)
