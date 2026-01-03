# Meta-target to build all libraries at once
add_custom_target(all_libraries
	DEPENDS
		bayesian_network
		decision_tree
		deep_learning
		networking_http_request_response
		data_structures
	# Add more library targets here as needed
)
# Machine Learning _Libraries
add_subdirectory(ml/bayesian_network)
add_subdirectory(ml/decision_tree)
add_subdirectory(ml/deep_learning)
add_subdirectory(ml/computer_vision)
add_subdirectory(ml/dimensionality_reduction)
add_subdirectory(ml/distribution)
add_subdirectory(ml/tracker)
add_subdirectory(ml/hidden_markov_model)
add_subdirectory(ml/generalized_linear_model)
add_subdirectory(ml/multi_arm_bandit)
add_subdirectory(ml/support_vector_machine)
add_subdirectory(ml/latent_sentiment_analysis)
add_subdirectory(ml/marked_point_process)
add_subdirectory(ml/time_series)
add_subdirectory(ml/nlp)

# Networking _Libraries
add_subdirectory(networking/http)
add_subdirectory(networking/distributed)
add_subdirectory(networking/rest_api)
add_subdirectory(networking/html)

# Data Format _Libraries
add_subdirectory(dataformats/json)

# Database _Libraries
add_subdirectory(databases/sql)

# Data Structures Library
add_subdirectory(data_structures)

# Services
add_subdirectory(services)

# ML Server
add_subdirectory(ml_server)

# Security
add_subdirectory(security)

# Authentication
add_subdirectory(auth)

# System Tools
add_subdirectory(system)

# File System (FileWatcher)
add_subdirectory(file_system)