include(CTest)

if(BUILD_TESTING)
    add_subdirectory(_deliverables/libraries/groups/Generics/tyst_framework)
endif()

set(movie_editor_library_dirs
    app_builder/GRAPHICS
    trekker/IO/dataformats/json
    app_builder/OS_GENERICS/windows
    app_builder/OS_GENERICS/full_application_window
    app_builder/OS_GENERICS/abstract_dialog
    app_builder/OS_GENERICS/os_dialog
    app_builder/OS_GENERICS/cli_tools
    audio_visual_group/VIDEO_ASSETS/audio_processing
    audio_visual_group/VIDEO_ASSETS/audio_container
    audio_visual_group/VIDEO_ASSETS/video_codec
    audio_visual_group/VIDEO_ASSETS/video_container
    audio_visual_group/VIDEO_ASSETS/video_timeline
    audio_visual_group/VIDEO_ASSETS/movie_editor_core
)

foreach(library_dir IN LISTS movie_editor_library_dirs)
    add_subdirectory("_deliverables/libraries/groups/${library_dir}")
endforeach()

add_subdirectory(_deliverables/apps/movie_editor)
