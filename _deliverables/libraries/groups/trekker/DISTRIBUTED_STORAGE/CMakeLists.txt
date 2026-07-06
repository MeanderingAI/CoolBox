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
    add_executable(distributed_storage_block_store_test block_store/tests/test_block_store.cpp)
    target_link_libraries(distributed_storage_block_store_test PRIVATE distributed_storage Threads::Threads)
    target_include_directories(distributed_storage_block_store_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageBlockStoreTest COMMAND distributed_storage_block_store_test)

    add_executable(distributed_storage_metadata_server_test metadata_server/tests/test_metadata_server.cpp)
    target_link_libraries(distributed_storage_metadata_server_test PRIVATE distributed_storage Threads::Threads)
    target_include_directories(distributed_storage_metadata_server_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageMetadataServerTest COMMAND distributed_storage_metadata_server_test)

    add_executable(distributed_storage_dfs_client_test dfs_client/tests/test_dfs_client.cpp)
    target_link_libraries(distributed_storage_dfs_client_test PRIVATE distributed_storage Threads::Threads)
    target_include_directories(distributed_storage_dfs_client_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageDfsClientTest COMMAND distributed_storage_dfs_client_test)

    add_executable(distributed_storage_replication_test replication/tests/test_replication.cpp)
    target_link_libraries(distributed_storage_replication_test PRIVATE distributed_storage Threads::Threads)
    target_include_directories(distributed_storage_replication_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageReplicationTest COMMAND distributed_storage_replication_test)

    add_executable(distributed_storage_chunk_transfer_test chunk_transfer/tests/test_chunk_transfer.cpp)
    target_link_libraries(distributed_storage_chunk_transfer_test PRIVATE distributed_storage Threads::Threads)
    target_include_directories(distributed_storage_chunk_transfer_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/block_store/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_server/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/dfs_client/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/replication/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/chunk_transfer/headers
    )
    add_test(NAME DistributedStorageChunkTransferTest COMMAND distributed_storage_chunk_transfer_test)
endif()
