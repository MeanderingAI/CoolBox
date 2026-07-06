cmake_minimum_required(VERSION 3.16)
project(body_generator)

add_executable(body_generator
    src/main.cpp
    src/body_generator.cpp
)

target_include_directories(body_generator PRIVATE include)

# Link to 3D/graphics libraries here if needed
# target_link_libraries(body_generator ...)

add_executable(body_generator_ui
    src/ui_main.cpp
    src/body_generator.cpp
)
target_include_directories(body_generator_ui PRIVATE include)
target_include_directories(body_generator_ui PRIVATE ${CMAKE_SOURCE_DIR}/_deliverables/libraries/packages/GRAPHICS/full_application_window/headers)
target_link_libraries(body_generator_ui PRIVATE distribution full_application_window)

# Only link to the full_application_window target, which provides includes
