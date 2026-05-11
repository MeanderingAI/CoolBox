# Charts / Graphics Library

include(FetchContent)

set(CHARTS_LIBRARY_TYPE SHARED)
if(WIN32)
    # Avoid passing STL-owned objects like `Canvas` across a DLL boundary in tests.
    set(CHARTS_LIBRARY_TYPE STATIC)
endif()

# Fetch stb (single-header image write library) for PNG/JPG export
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

add_library(charts ${CHARTS_LIBRARY_TYPE}
    source/graphics.cpp
)

target_include_directories(charts PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

# stb_image_write.h lives in the stb source directory
# Mark as SYSTEM to suppress third-party warnings (e.g. sprintf deprecation)
target_include_directories(charts SYSTEM PRIVATE
    ${stb_SOURCE_DIR}
)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(charts_tests tests/test_graphics.cpp)
    target_link_libraries(charts_tests PRIVATE charts tyst_framework_main)
    add_test(NAME ChartsTests COMMAND charts_tests)
endif()
