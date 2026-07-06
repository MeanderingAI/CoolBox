cmake_minimum_required(VERSION 3.16)

# EMAIL view for shared email stack.
# Source folders are hosted in trekker/IO/EMAIL and wired here so they stay grouped
# under audio_visual_group/EMAIL like COMMS.
set(_trekker_io_email_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../trekker/IO/EMAIL")

add_subdirectory(
    "${_trekker_io_email_dir}/mailbox"
    "${CMAKE_BINARY_DIR}/audio_visual_email_mailbox_build"
)

add_subdirectory(
    "${_trekker_io_email_dir}/smtp_server"
    "${CMAKE_BINARY_DIR}/audio_visual_email_smtp_server_build"
)

add_subdirectory(
    "${_trekker_io_email_dir}/imap_server"
    "${CMAKE_BINARY_DIR}/audio_visual_email_imap_server_build"
)

add_subdirectory(
    "${_trekker_io_email_dir}/api_exposure"
    "${CMAKE_BINARY_DIR}/audio_visual_email_api_exposure_build"
)
