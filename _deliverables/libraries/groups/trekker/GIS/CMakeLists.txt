add_library(gis SHARED
    source/gis.cpp
    source/geojson.cpp
)
target_include_directories(gis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(gis PUBLIC cxx_std_17)
target_link_libraries(gis PUBLIC json)

if(WIN32)
    set_target_properties(gis PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(gis_tests tests/test_gis.cpp)
    target_link_libraries(gis_tests PRIVATE gis tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(gis_tests)
    add_test(NAME GisTests COMMAND gis_tests)
endif()
