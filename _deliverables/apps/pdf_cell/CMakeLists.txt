cmake_minimum_required(VERSION 3.20)
project(pdf_cell LANGUAGES CXX)

add_executable(pdf_cell main.cpp)
target_compile_features(pdf_cell PRIVATE cxx_std_17)
target_link_libraries(pdf_cell PRIVATE
    pdf_reader
    ocr_engine
    credit_card_statement
    xlsx_writer
)

install(TARGETS pdf_cell)
