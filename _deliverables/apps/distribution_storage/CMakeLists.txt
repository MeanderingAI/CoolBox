cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

set(SHARED_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/shared")

# Include paths for all distributed_storage headers
set(DS_HEADERS
    "${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/block_store/headers"
    "${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/metadata_server/headers"
    "${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/dfs_client/headers"
    "${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/replication/headers"
    "${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/chunk_transfer/headers"
)

# ── storage_node ──────────────────────────────────────────────────────────────
add_executable(distribution_storage_node
    storage_node/main.cpp
)
target_include_directories(distribution_storage_node PRIVATE
    ${SHARED_INCLUDE}
    ${DS_HEADERS}
)
target_link_libraries(distribution_storage_node PRIVATE
    distributed_storage
    Threads::Threads
)
set_target_properties(distribution_storage_node PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# ── metadata_node ─────────────────────────────────────────────────────────────
add_executable(distribution_storage_meta
    metadata_node/main.cpp
)
target_include_directories(distribution_storage_meta PRIVATE
    ${SHARED_INCLUDE}
    ${DS_HEADERS}
)
target_link_libraries(distribution_storage_meta PRIVATE
    distributed_storage
    Threads::Threads
)
set_target_properties(distribution_storage_meta PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# ── dfs_shell ─────────────────────────────────────────────────────────────────
add_executable(distribution_storage_shell
    dfs_shell/main.cpp
)
target_include_directories(distribution_storage_shell PRIVATE
    ${SHARED_INCLUDE}
    ${DS_HEADERS}
)
target_link_libraries(distribution_storage_shell PRIVATE
    distributed_storage
    Threads::Threads
)
set_target_properties(distribution_storage_shell PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
