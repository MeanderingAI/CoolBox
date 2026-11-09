add_executable(test_llm
    test/test_llm.cpp
    src/tokenizer.cpp
    src/decoder_language_model.cpp
    ../layers/src/tensor.cpp
    ../layers/src/layer.cpp
)

target_include_directories(test_llm PRIVATE
    include
    ../layers/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
target_compile_features(test_llm PRIVATE cxx_std_17)

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../../cmake/VulkanTensorBackend.cmake)
coolbox_configure_vulkan_tensor(test_llm)

add_test(NAME DecoderLanguageModelTests COMMAND test_llm)
