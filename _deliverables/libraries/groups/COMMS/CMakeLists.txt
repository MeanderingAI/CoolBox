cmake_minimum_required(VERSION 3.20)

project(COMMS VERSION 1.0 LANGUAGES C CXX)


add_subdirectory(LSP)
add_subdirectory(PARSER)
add_subdirectory(DOCUMENTS)

# Example: link tyst_framework_main to your targets if needed
# target_link_libraries(your_target PUBLIC tyst_framework_main)