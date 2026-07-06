cmake_minimum_required(VERSION 3.16)

# COMMS view for shared transport and servlet stack used by audio/voip services.
# Source folders remain in trekker/IO but are wired here so they are grouped under COMMS.
set(_trekker_io_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../trekker/IO")

add_subdirectory(
    "${_trekker_io_dir}/quic_transport"
    "${CMAKE_BINARY_DIR}/audio_visual_comms_quic_transport_build"
)

add_subdirectory(
    "${_trekker_io_dir}/servlets"
    "${CMAKE_BINARY_DIR}/audio_visual_comms_servlets_build"
)

add_subdirectory(
    "${_trekker_io_dir}/http_server"
    "${CMAKE_BINARY_DIR}/audio_visual_comms_http_server_build"
)
