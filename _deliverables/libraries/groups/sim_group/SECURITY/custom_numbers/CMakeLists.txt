include(CheckCXXSourceCompiles)

add_library(custom_numbers SHARED
    source/custom_numbers.cpp
)
target_include_directories(custom_numbers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(custom_numbers PUBLIC cxx_std_17)

check_cxx_source_compiles("\
    #include <charconv>\n\
    int main() {\n\
        char output[64];\n\
        double value = 1.25;\n\
        auto written = std::to_chars(output, output + sizeof(output), value);\n\
        return std::from_chars(output, written.ptr, value).ec == std::errc{} ? 0 : 1;\n\
    }"
    CUSTOM_NUMBERS_FLOAT_CHARCONV_SUPPORTED)

if(CUSTOM_NUMBERS_FLOAT_CHARCONV_SUPPORTED)
    target_compile_definitions(custom_numbers PUBLIC CUSTOM_NUMBERS_HAS_FLOAT_CHARCONV=1)
endif()

option(CUSTOM_NUMBERS_ENABLE_OPTIONAL_FLOAT_CONVERTERS
    "Use installed double-conversion, Abseil, and fast_float packages" ON)

if(CUSTOM_NUMBERS_ENABLE_OPTIONAL_FLOAT_CONVERTERS)
    find_package(double-conversion CONFIG QUIET)
    if(TARGET double-conversion::double-conversion)
        target_link_libraries(custom_numbers PRIVATE double-conversion::double-conversion)
        target_compile_definitions(custom_numbers PRIVATE CUSTOM_NUMBERS_HAS_DOUBLE_CONVERSION=1)
    endif()

    find_package(absl CONFIG QUIET)
    if(TARGET absl::strings)
        target_link_libraries(custom_numbers PRIVATE absl::strings)
        target_compile_definitions(custom_numbers PRIVATE CUSTOM_NUMBERS_HAS_ABSEIL=1)
    endif()

    find_package(FastFloat CONFIG QUIET)
    if(TARGET FastFloat::fast_float)
        target_link_libraries(custom_numbers PRIVATE FastFloat::fast_float)
        target_compile_definitions(custom_numbers PRIVATE CUSTOM_NUMBERS_HAS_FAST_FLOAT=1)
    elseif(TARGET fast_float)
        target_link_libraries(custom_numbers PRIVATE fast_float)
        target_compile_definitions(custom_numbers PRIVATE CUSTOM_NUMBERS_HAS_FAST_FLOAT=1)
    endif()
endif()

add_executable(custom_numbers_tests
    tests/custom_numbers_test.cpp
)
target_link_libraries(custom_numbers_tests PRIVATE custom_numbers)
add_test(NAME CustomNumbersTests COMMAND custom_numbers_tests)