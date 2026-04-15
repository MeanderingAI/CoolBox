cmake_minimum_required(VERSION 3.15)
project(MStudio LANGUAGES CXX)

set(MSTUDIO_SOURCES
	src/main.cpp
	src/gui_gtk_like.cpp
	src/text_box_editor.cpp
)

add_executable(MStudio ${MSTUDIO_SOURCES})
target_compile_features(MStudio PRIVATE cxx_std_17)
target_include_directories(MStudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(TARGET components)
	target_link_libraries(MStudio PRIVATE components)
else()
	target_sources(MStudio PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../_libraries/packages/GRAPHICS/components/source/components.cpp
	)
	target_include_directories(MStudio PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../../_libraries/packages/GRAPHICS/components/headers
	)
endif()

if(TARGET full_application_window)
	target_link_libraries(MStudio PRIVATE full_application_window)
endif()

if(TARGET file_browser_lib)
	target_link_libraries(MStudio PRIVATE file_browser_lib)
endif()

if(TARGET installer_abstraction)
	target_link_libraries(MStudio PRIVATE installer_abstraction)
	target_compile_definitions(MStudio PRIVATE COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS=1)
endif()

if(WIN32)
	if(TARGET full_application_window)
		add_custom_command(TARGET MStudio POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$<TARGET_FILE:full_application_window>
				$<TARGET_FILE_DIR:MStudio>
		)
	endif()
	if(TARGET components)
		add_custom_command(TARGET MStudio POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$<TARGET_FILE:components>
				$<TARGET_FILE_DIR:MStudio>
		)
	endif()
	if(TARGET json)
		add_custom_command(TARGET MStudio POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$<TARGET_FILE:json>
				$<TARGET_FILE_DIR:MStudio>
		)
	endif()
	if(TARGET file_browser_lib)
		add_custom_command(TARGET MStudio POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				$<TARGET_FILE:file_browser_lib>
				$<TARGET_FILE_DIR:MStudio>
		)
	endif()
endif()
