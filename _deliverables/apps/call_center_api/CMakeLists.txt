cmake_minimum_required(VERSION 3.20)
project(call_center_api LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CALL_CENTER_API_INCLUDES
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/http_server/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/servlets/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/audio_visual_group/VOIP/sip_server/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/audio_visual_group/VOIP/mcu/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/audio_visual_group/VOIP/nat_traversal/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/audio_visual_group/VOIP/api_exposure/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/audio_visual_group/VOIP/freeswitch_adapter/headers
)

add_executable(call_center_api main.cpp)
target_compile_features(call_center_api PRIVATE cxx_std_17)
target_include_directories(call_center_api PRIVATE ${CALL_CENTER_API_INCLUDES})
target_link_libraries(call_center_api PRIVATE
    sip_server
    voip_mcu
    nat_traversal
    voip_api_exposure
    voip_freeswitch_adapter
    http_server
    servlets
    Threads::Threads
)

install(TARGETS call_center_api)
