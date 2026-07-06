cmake_minimum_required(VERSION 3.16)
project(CoolBoxXamarinBindings LANGUAGES C CXX)

add_library(coolbox_xamarin_bindings SHARED
    src/coolbox_xamarin_bindings.cpp
)

set_target_properties(coolbox_xamarin_bindings PROPERTIES
    OUTPUT_NAME coolbox_xamarin_bindings
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(coolbox_xamarin_bindings PRIVATE cxx_std_17)

target_compile_definitions(coolbox_xamarin_bindings PRIVATE
    COOLBOX_XAMARIN_BINDINGS_EXPORTS
)

if(WIN32)
    target_compile_definitions(coolbox_xamarin_bindings PUBLIC
        COOLBOX_XAMARIN_BINDINGS_SHARED
    )
endif()

target_include_directories(coolbox_xamarin_bindings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../c_bindings/include>
)

target_link_libraries(coolbox_xamarin_bindings
    PRIVATE
        coolbox_c_bindings
        liquid_ui_lib
)
