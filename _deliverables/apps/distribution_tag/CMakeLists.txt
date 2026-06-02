cmake_minimum_required(VERSION 3.20)
project(distribution_tag LANGUAGES CXX)

find_package(Threads REQUIRED)

set(SHARED_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/shared)
set(DIST_COMP_ROOT ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/DISTRIBUTED_COMPUTE)
set(IO_ROOT ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO)

set(DIST_COMP_INCLUDES
	${DIST_COMP_ROOT}/circuit_breaker/headers
	${DIST_COMP_ROOT}/distributed_lock/headers
	${DIST_COMP_ROOT}/message_bus/headers
	${DIST_COMP_ROOT}/service_registry/headers
	${DIST_COMP_ROOT}/task_queue/headers
)

set(DIST_TAG_IO_INCLUDES
	${IO_ROOT}/http_server/headers
	${IO_ROOT}/advanced_logging/headers
)

# ── Master node ────────────────────────────────────────────────────────────────
add_executable(distribution_tag_master master/main.cpp)
target_compile_features(distribution_tag_master PRIVATE cxx_std_17)
target_include_directories(distribution_tag_master PRIVATE
	${SHARED_INCLUDE}
	${DIST_COMP_INCLUDES}
	${DIST_TAG_IO_INCLUDES}
)
target_link_libraries(distribution_tag_master PRIVATE distributed_compute http_server Threads::Threads)

# ── Worker node ────────────────────────────────────────────────────────────────
add_executable(distribution_tag_worker worker/main.cpp)
target_compile_features(distribution_tag_worker PRIVATE cxx_std_17)
target_include_directories(distribution_tag_worker PRIVATE
	${SHARED_INCLUDE}
	${DIST_COMP_INCLUDES}
)
target_link_libraries(distribution_tag_worker PRIVATE distributed_compute Threads::Threads)

install(TARGETS distribution_tag_master distribution_tag_worker)
