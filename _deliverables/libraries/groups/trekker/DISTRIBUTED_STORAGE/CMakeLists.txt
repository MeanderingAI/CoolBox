# Distributed Storage Library (trekker::dfs)
# Subpackages: block_store | metadata_server | dfs_client | replication | chunk_transfer

set(DISTRIBUTED_STORAGE_SOURCES
    block_store/source/block_store.cpp
    block_store/source/block_map.cpp
    metadata_server/source/metadata_server.cpp
    metadata_server/source/dir_tree.cpp
    dfs_client/source/dfs_client.cpp
    dfs_client/source/file_handle.cpp
    replication/source/replication.cpp
    chunk_transfer/source/chunk_transfer.cpp
)

add_library(distributed_storage ${DISTRIBUTED_STORAGE_SOURCES})

target_include_directories(distributed_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
)

target_compile_features(distributed_storage PUBLIC cxx_std_17)

find_package(Threads REQUIRED)
target_link_libraries(distributed_storage PRIVATE Threads::Threads)

if(BUILD_TESTING)
    add_executable(distributed_storage_tests
        block_store/tests/test_block_store.cpp
        metadata_server/tests/test_metadata_server.cpp
        dfs_client/tests/test_dfs_client.cpp
        replication/tests/test_replication.cpp
        chunk_transfer/tests/test_chunk_transfer.cpp
    )
    target_link_libraries(distributed_storage_tests PRIVATE
        distributed_storage tyst_framework_main Threads::Threads)
    target_include_directories(distributed_storage_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageTests COMMAND distributed_storage_tests)
endif()
