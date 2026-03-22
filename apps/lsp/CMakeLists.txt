cmake_minimum_required(VERSION 3.12)
project(plang_lsp_app)

add_executable(plang_lsp src/main_plang.cpp)
target_link_libraries(plang_lsp PRIVATE plang_lsp_lib)

add_executable(plrust_lsp src/main_rust.cpp)
target_link_libraries(plrust_lsp PRIVATE plrust_lsp_lib)

add_executable(pljava_lsp src/main_java.cpp)
target_link_libraries(pljava_lsp PRIVATE pljava_lsp_lib)

add_executable(plpython_lsp src/main_python.cpp)
target_link_libraries(plpython_lsp PRIVATE plpython_lsp_lib)

install(TARGETS plang_lsp plrust_lsp pljava_lsp plpython_lsp)
