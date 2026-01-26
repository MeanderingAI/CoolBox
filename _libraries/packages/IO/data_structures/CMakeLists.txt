# Data Structures Library

add_library(data_structures SHARED
    source/binary_search_tree.cpp
    source/linked_list.cpp
    source/hash_map.cpp
    source/concurrent_hash_map.cpp
    source/concurrent_linked_list.cpp
)

target_include_directories(data_structures PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/service_cache/__include
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/service_dns/__include
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/__rej_proxy_service/include
)

target_compile_features(data_structures PUBLIC cxx_std_17)

# Link pthread for concurrent data structures
find_package(Threads REQUIRED)
target_link_libraries(data_structures PRIVATE Threads::Threads)
