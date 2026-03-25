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

# VHDL and MATLAB LSP frontends
add_executable(plvhdl_lsp src/main_vhdl.cpp)
target_link_libraries(plvhdl_lsp PRIVATE plvhdl_lsp_lib)
target_include_directories(plvhdl_lsp PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/backages/TOOLS/lsp_vhdl/headers)
target_link_libraries(plvhdl_lsp PRIVATE vhdl_parser)

add_executable(plmatlab_lsp src/main_matlab.cpp)
target_link_libraries(plmatlab_lsp PRIVATE plmatlab_lsp_lib)
target_include_directories(plmatlab_lsp PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/backages/TOOLS/lsp_matlab/headers)
target_link_libraries(plmatlab_lsp PRIVATE matlab_parser)

install(TARGETS plang_lsp plrust_lsp pljava_lsp plpython_lsp)
install(TARGETS plvhdl_lsp plmatlab_lsp)
