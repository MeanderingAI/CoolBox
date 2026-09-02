cmake_minimum_required(VERSION 3.12)
project(pdf_cell_studio_app LANGUAGES CXX)

add_executable(pdf_cell_studio
    src/main.cpp
)

target_link_libraries(pdf_cell_studio PRIVATE
    full_application_window
    components
    os_dialog
    pdf_reader
    ocr_engine
    credit_card_statement
    xlsx_writer
)

target_compile_features(pdf_cell_studio PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET pdf_cell_studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:pdf_cell_studio>
            $<TARGET_FILE_DIR:pdf_cell_studio>
        COMMAND_EXPAND_LISTS
    )
endif()
