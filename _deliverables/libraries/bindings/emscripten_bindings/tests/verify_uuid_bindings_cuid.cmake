if(NOT DEFINED UUID_BINDINGS_FILE)
    message(FATAL_ERROR "UUID_BINDINGS_FILE was not provided")
endif()

if(NOT EXISTS "${UUID_BINDINGS_FILE}")
    message(FATAL_ERROR "UUID bindings file not found: ${UUID_BINDINGS_FILE}")
endif()

file(READ "${UUID_BINDINGS_FILE}" BINDINGS_CONTENT)

if(NOT BINDINGS_CONTENT MATCHES "js_cuid")
    message(FATAL_ERROR "Missing js_cuid helper in uuid_generation_bindings.cpp")
endif()

if(NOT BINDINGS_CONTENT MATCHES "function\\(\\\"cuid\\\",[ \\t]*&js_cuid\\)")
    message(FATAL_ERROR "Missing EMSCRIPTEN_BINDINGS export for function(\"cuid\", &js_cuid)")
endif()

if(NOT BINDINGS_CONTENT MATCHES "generate_cuid\\(\\)")
    message(FATAL_ERROR "Missing generate_cuid() call in uuid_generation_bindings.cpp")
endif()

message(STATUS "UUID emscripten bindings CUID smoke check passed")
