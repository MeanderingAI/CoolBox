cmake_minimum_required(VERSION 3.16)

# audio_visual_group — parent group that discovers VIDEO_ASSETS and MUSIC_ASSETS.
file(GLOB _children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */CMakeLists.txt)
foreach(_child_cmake ${_children})
    get_filename_component(_pkg ${_child_cmake} DIRECTORY)
    add_subdirectory(${_pkg})
endforeach()
