option(
    COOLBOX_ENABLE_VULKAN_TENSOR
    "Enable the Vulkan compute backend for deep-learning tensors"
    ON
)

get_filename_component(
    COOLBOX_VULKAN_TENSOR_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/.."
    ABSOLUTE
)

function(coolbox_configure_vulkan_tensor target)
    if(NOT COOLBOX_ENABLE_VULKAN_TENSOR)
        return()
    endif()

    find_package(Vulkan QUIET)
    find_program(
        COOLBOX_GLSLANG_VALIDATOR
        NAMES glslangValidator
        HINTS
            "$ENV{VULKAN_SDK}/bin"
            "$ENV{VULKAN_SDK}/Bin"
    )

    if(NOT Vulkan_FOUND OR NOT COOLBOX_GLSLANG_VALIDATOR)
        message(STATUS
            "Vulkan tensor backend disabled for ${target}: Vulkan headers, "
            "loader, or glslangValidator was not found")
        return()
    endif()

    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/${target}_vulkan_generated")
    file(MAKE_DIRECTORY "${generated_dir}")

    set(shader_headers)
    foreach(precision IN ITEMS fp64 fp32 lowp)
        if(precision STREQUAL "fp64")
            set(shader_name "tensor_ops")
        else()
            set(shader_name "tensor_ops_${precision}")
        endif()
        set(shader_source
            "${COOLBOX_VULKAN_TENSOR_ROOT}/_deliverables/libraries/groups/cool_car/DL/layers/shaders/${shader_name}.comp")
        set(shader_binary "${generated_dir}/${shader_name}.spv")
        set(shader_header
            "${generated_dir}/vulkan_tensor_shader_${precision}.h")
        add_custom_command(
            OUTPUT "${shader_header}"
            COMMAND "${COOLBOX_GLSLANG_VALIDATOR}" -V "${shader_source}" -o "${shader_binary}"
            COMMAND "${CMAKE_COMMAND}"
                "-DINPUT=${shader_binary}"
                "-DOUTPUT=${shader_header}"
                "-DSYMBOL=coolbox_tensor_ops_${precision}_spv"
                -P "${COOLBOX_VULKAN_TENSOR_ROOT}/cmake/EmbedBinary.cmake"
            DEPENDS
                "${shader_source}"
                "${COOLBOX_VULKAN_TENSOR_ROOT}/cmake/EmbedBinary.cmake"
            VERBATIM
        )
        list(APPEND shader_headers "${shader_header}")
    endforeach()

    target_sources(${target} PRIVATE
        "${COOLBOX_VULKAN_TENSOR_ROOT}/_deliverables/libraries/groups/cool_car/DL/layers/src/vulkan_tensor_backend.cpp"
        ${shader_headers}
    )
    target_include_directories(${target} PRIVATE "${generated_dir}")
    target_compile_definitions(${target} PRIVATE COOLBOX_HAS_VULKAN_TENSOR=1)
    target_link_libraries(${target} PRIVATE Vulkan::Vulkan)
    message(STATUS "Vulkan tensor backend enabled for ${target}")
endfunction()
