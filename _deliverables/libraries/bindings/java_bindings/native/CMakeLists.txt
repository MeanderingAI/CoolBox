# Standalone native build for the CoolBox Java (JNA) bindings.
#
# Produces, in COOLBOX_JAVA_NATIVE_OUTPUT_DIR:
#   * coolbox_java_bridge - flat C ABI over the ML, data-structure, matrix,
#     optimisation, simulation and SQL helpers exposed to Python.
#   * coolbox_c_bindings  - the existing C ABI for SQL, HTTP, forms and UUIDs.
#   * every runtime DLL they need, so the JVM only has to be pointed at one
#     directory.
# This project deliberately does not go through the repository root
# CMakeLists.txt so it keeps working without the root's optional dependencies.
cmake_minimum_required(VERSION 3.21)
project(CoolBoxJavaBridge LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

get_filename_component(COOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(COOLBOX_LIB "${COOLBOX_ROOT}/_deliverables/libraries")
set(COOLBOX_GROUPS "${COOLBOX_LIB}/groups")
set(COOLBOX_PY "${COOLBOX_LIB}/bindings/python_bindings")
set(COOLBOX_GO "${COOLBOX_LIB}/bindings/go_bindings")
set(COOLBOX_CB "${COOLBOX_LIB}/bindings/c_bindings")

set(COOLBOX_JAVA_NATIVE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/natives" CACHE PATH
    "Directory that receives coolbox_java_bridge, coolbox_c_bindings and their runtime DLLs")
set(COOLBOX_JAVA_BRIDGE_MODULES "" CACHE STRING
    "Optional ;-list restricting which native/src/<module>_jbridge.cc files are built (empty = all)")

# --- Third-party dependencies (fetched, so nothing has to be preinstalled) ---
if(NOT TARGET Eigen3::Eigen)
    FetchContent_Declare(
        eigen
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR _coolbox_header_only
    )
    FetchContent_MakeAvailable(eigen)
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${eigen_SOURCE_DIR}")
endif()

FetchContent_Declare(
    stb
    URL https://github.com/nothings/stb/archive/31c1ad37456438565541f4919958214b6e762fb4.zip
    URL_HASH SHA256=617266695cf191a45bec2405427207011a09b057133134594b0db6ccbf9ee0b2
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(stb)

# The SQL helpers call find_package(SQLite3 REQUIRED). Unless the caller asks
# for the system SQLite, build the public-domain amalgamation and pre-create
# the targets FindSQLite3 would otherwise import.
option(COOLBOX_JAVA_USE_SYSTEM_SQLITE "Use an installed SQLite3 instead of the bundled amalgamation" OFF)
if(NOT COOLBOX_JAVA_USE_SYSTEM_SQLITE AND NOT TARGET SQLite::SQLite3)
    FetchContent_Declare(
        coolbox_sqlite3
        URL https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
        URL_HASH SHA3_256=f131b68e6ba5fb891cc13ebb5ff9555054c77294cb92d8d1268bad5dba4fa2a1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(coolbox_sqlite3)
    add_library(coolbox_bundled_sqlite3 STATIC "${coolbox_sqlite3_SOURCE_DIR}/sqlite3.c")
    target_include_directories(coolbox_bundled_sqlite3 PUBLIC "${coolbox_sqlite3_SOURCE_DIR}")
    target_compile_definitions(coolbox_bundled_sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION)
    add_library(SQLite::SQLite3 ALIAS coolbox_bundled_sqlite3)
    add_library(SQLite3::SQLite3 ALIAS coolbox_bundled_sqlite3)
    set(SQLite3_INCLUDE_DIR "${coolbox_sqlite3_SOURCE_DIR}" CACHE PATH "" FORCE)
    set(SQLite3_LIBRARY coolbox_bundled_sqlite3 CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

# --- coolbox_c_bindings (SQL, HTTP, forms/HTML, UUID) ---
set(BUILD_TESTING OFF)
add_subdirectory("${COOLBOX_CB}" "${CMAKE_BINARY_DIR}/c_bindings")

# --- coolbox_java_bridge ---
set(TREE_ENSEMBLE_ROOT "${COOLBOX_GROUPS}/cool_car/ML/decision_tree")
foreach(header boost_tree.h random_forest_regressor.h decision_tree_regressor.h tree_ensemble_utils.h)
    configure_file("${TREE_ENSEMBLE_ROOT}/headers/${header}"
        "${CMAKE_BINARY_DIR}/tree_ensemble_include/tree_ensemble/${header}" COPYONLY)
endforeach()

file(GLOB COOLBOX_PY_IMPL_SOURCES CONFIGURE_DEPENDS
    "${COOLBOX_PY}/src/generalized_linear_model/*.cpp"
    "${COOLBOX_PY}/src/distributed/*.cpp"
    "${COOLBOX_PY}/src/nlp/*.cpp"
    "${COOLBOX_PY}/src/dimensionality_reduction/*.cpp"
    "${COOLBOX_PY}/src/decision_tree/*.cpp"
    "${COOLBOX_PY}/src/support_vector_machine/*.cpp"
    "${COOLBOX_PY}/src/bayesian_network/*.cpp"
    "${COOLBOX_PY}/src/hidden_markov_model/*.cpp"
    "${COOLBOX_PY}/src/multi_arm_bandit/*.cpp"
    "${COOLBOX_PY}/src/computer_vision/*.cpp"
    "${COOLBOX_PY}/src/time_series/*.cpp"
    "${COOLBOX_PY}/src/tracker/*.cpp"
)
list(FILTER COOLBOX_PY_IMPL_SOURCES EXCLUDE REGEX "/(main|test_[^/]*)\\.cpp$")

set(COOLBOX_CORE_SOURCES
    ${COOLBOX_PY_IMPL_SOURCES}
    "${TREE_ENSEMBLE_ROOT}/source/decision_tree_regressor.cpp"
    "${TREE_ENSEMBLE_ROOT}/source/boost_tree.cpp"
    "${TREE_ENSEMBLE_ROOT}/source/random_forest_regressor.cpp"
    "${COOLBOX_PY}/vendor_src/GRAPHICS/charts/source/graphics.cpp"
    "${COOLBOX_PY}/vendor_src/MISC/wave_generator/source/wave_generator.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/layers/src/tensor.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/layers/src/layer.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/loss/src/loss.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/optimizer/src/optimizer.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/wrapper/src/neural_network.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/wrapper/src/templates.cpp"
    "${COOLBOX_GROUPS}/cool_car/DL/agents/src/llf_helix.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/bayesian_quantum_control/source/quantum_control_bo.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/genetic_search/source/genetic_search.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/hill_climbing/source/hill_climbing.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/optimization_factory/source/optimization_factory.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/simulated_annealing/source/simulated_annealing.cpp"
    "${COOLBOX_GROUPS}/cool_car/OPT/distributed_learning/source/distributed_owa.cpp"
    "${COOLBOX_GROUPS}/cool_car/FINANCE/source/finance.cpp"
    "${COOLBOX_GROUPS}/sim_group/ELECTRONICS/circuitry/src/circuit.cpp"
)
# Single TU: it also explicitly instantiates the int64 containers used by ds_bridge.cc.
set(COOLBOX_DS_SOURCES "${COOLBOX_GO}/ds_bridge_sources.cc")

# C ABI bridges shared with the Go bindings. The cgo-only trampolines
# (opt_math_finance_go_trampoline.cc, go_http_callback.cc) and the *_sources.cc
# amalgamations are intentionally excluded.
set(COOLBOX_GO_BRIDGES ds matrix dl opt_math_finance graphics glm tree bandit forecast series nlp)
set(COOLBOX_GO_BRIDGE_SOURCES "")
foreach(bridge IN LISTS COOLBOX_GO_BRIDGES)
    list(APPEND COOLBOX_GO_BRIDGE_SOURCES "${COOLBOX_GO}/${bridge}_bridge.cc")
endforeach()

# Java-specific bridges for APIs that have no Go equivalent.
file(GLOB COOLBOX_JAVA_GAP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*_jbridge.cc")
if(COOLBOX_JAVA_BRIDGE_MODULES)
    set(_selected "${CMAKE_CURRENT_SOURCE_DIR}/src/core_jbridge.cc")
    foreach(module IN LISTS COOLBOX_JAVA_BRIDGE_MODULES)
        list(APPEND _selected "${CMAKE_CURRENT_SOURCE_DIR}/src/${module}_jbridge.cc")
    endforeach()
    list(REMOVE_DUPLICATES _selected)
    set(COOLBOX_JAVA_GAP_SOURCES ${_selected})
endif()

set(COOLBOX_BRIDGE_SOURCES ${COOLBOX_GO_BRIDGE_SOURCES} ${COOLBOX_JAVA_GAP_SOURCES})

add_library(coolbox_java_bridge SHARED
    ${COOLBOX_CORE_SOURCES}
    ${COOLBOX_DS_SOURCES}
    ${COOLBOX_BRIDGE_SOURCES}
)

target_include_directories(coolbox_java_bridge PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${COOLBOX_GO}"
    "${COOLBOX_ROOT}/include"
    "${COOLBOX_PY}/include"
    "${COOLBOX_PY}/include/bayesian_network"
    "${COOLBOX_PY}/include/computer_vision"
    "${COOLBOX_PY}/include/decision_tree"
    "${COOLBOX_PY}/include/dimensionality_reduction"
    "${COOLBOX_PY}/include/distributed"
    "${COOLBOX_PY}/include/generalized_linear_model"
    "${COOLBOX_PY}/include/graphics_misc"
    "${COOLBOX_PY}/include/hidden_markov_model"
    "${COOLBOX_PY}/include/multi_arm_bandit"
    "${COOLBOX_PY}/include/nlp"
    "${COOLBOX_PY}/include/support_vector_machine"
    "${COOLBOX_PY}/include/time_series"
    "${COOLBOX_PY}/include/tracker"
    "${CMAKE_BINARY_DIR}/tree_ensemble_include"
    "${TREE_ENSEMBLE_ROOT}/headers"
    "${COOLBOX_GROUPS}/cool_car"
    "${COOLBOX_GROUPS}/cool_car/DL/layers/include"
    "${COOLBOX_GROUPS}/cool_car/DL/loss/include"
    "${COOLBOX_GROUPS}/cool_car/DL/optimizer/include"
    "${COOLBOX_GROUPS}/cool_car/DL/wrapper/include"
    "${COOLBOX_GROUPS}/cool_car/DL/agents/include"
    "${COOLBOX_GROUPS}/cool_car/MATRIX/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/bayesian_quantum_control/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/distributed_learning/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/genetic_search/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/hill_climbing/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/optimization_factory/headers"
    "${COOLBOX_GROUPS}/cool_car/OPT/simulated_annealing/headers"
    "${COOLBOX_GROUPS}/cool_car/FINANCE/headers"
    "${COOLBOX_GROUPS}/cool_car/MATH"
    "${COOLBOX_GROUPS}/Generics/timer/headers"
    "${COOLBOX_GROUPS}/sim_group/ELECTRONICS/battery/include"
    "${COOLBOX_GROUPS}/sim_group/ELECTRONICS/circuitry/include"
    "${COOLBOX_GROUPS}/trekker/DATASTRUCTURE/trees/headers"
    "${COOLBOX_GROUPS}/trekker/DATASTRUCTURE/maps/headers"
    "${COOLBOX_GROUPS}/trekker/DATASTRUCTURE/list/headers"
    "${COOLBOX_GROUPS}/trekker/DATASTRUCTURE/filters/headers"
    "${COOLBOX_GROUPS}/trekker/DATASTRUCTURE/xarray/headers"
    "${COOLBOX_PY}/vendor_include"
    "${COOLBOX_PY}/vendor_include/GRAPHICS/charts/headers"
    "${COOLBOX_PY}/vendor_include/MISC/wave_generator/headers"
)
target_include_directories(coolbox_java_bridge SYSTEM PRIVATE "${stb_SOURCE_DIR}")
target_link_libraries(coolbox_java_bridge PRIVATE sql json Eigen3::Eigen Threads::Threads)
if(WIN32)
    target_link_libraries(coolbox_java_bridge PRIVATE ws2_32 shell32)
endif()
if(MSVC)
    target_compile_options(coolbox_java_bridge PRIVATE /bigobj /EHsc /utf-8)
elseif(MINGW)
    target_compile_options(coolbox_java_bridge PRIVATE -Wa,-mbig-obj)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(coolbox_java_bridge PRIVATE -Wno-deprecated-declarations)
endif()

# Only the flat C entry points are exported on Windows. Exporting every symbol
# of a library this size risks overflowing the 65535-entry export table.
if(WIN32)
    set(_exports "")
    foreach(source IN LISTS COOLBOX_BRIDGE_SOURCES)
        file(STRINGS "${source}" _lines REGEX "^(extern \"C\"[ \t]+)?[A-Za-z_][A-Za-z0-9_ \t\\*]*[ \t\\*]coolbox_[A-Za-z0-9_]+[ \t]*\\(")
        foreach(line IN LISTS _lines)
            if(line MATCHES "^(static|inline|return|typedef)[ \t]")
                continue()
            endif()
            string(REGEX MATCH "coolbox_[A-Za-z0-9_]+[ \t]*\\(" _sym "${line}")
            string(REGEX REPLACE "[ \t]*\\($" "" _sym "${_sym}")
            list(APPEND _exports "${_sym}")
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _exports)
    list(SORT _exports)
    list(LENGTH _exports _export_count)
    message(STATUS "coolbox_java_bridge exports ${_export_count} C functions")
    list(JOIN _exports "\n    " _export_lines)
    file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/coolbox_java_bridge.def"
         CONTENT "LIBRARY coolbox_java_bridge\nEXPORTS\n    ${_export_lines}\n")
    target_sources(coolbox_java_bridge PRIVATE "${CMAKE_BINARY_DIR}/coolbox_java_bridge.def")
endif()

set_target_properties(coolbox_java_bridge PROPERTIES
    OUTPUT_NAME coolbox_java_bridge
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
)
foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(coolbox_java_bridge PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
    )
endforeach()

# Stage coolbox_c_bindings and all shared dependencies next to the bridge.
set(_stage_items
    "$<TARGET_FILE:coolbox_c_bindings>"
    "$<TARGET_RUNTIME_DLLS:coolbox_c_bindings>"
    "$<TARGET_RUNTIME_DLLS:coolbox_java_bridge>"
)
if(MINGW)
    get_filename_component(_cxx_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_cxx_name "${CMAKE_CXX_COMPILER}" NAME)
    string(REGEX REPLACE "-(clang\\+\\+|g\\+\\+)(\\.exe)?$" "" _triple "${_cxx_name}")
    foreach(runtime IN ITEMS libc++.dll libunwind.dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
        unset(_runtime_path CACHE)
        find_file(_runtime_path "${runtime}"
            HINTS "${_cxx_bin}/../${_triple}/bin" "${_cxx_bin}"
            NO_DEFAULT_PATH)
        if(_runtime_path)
            list(APPEND _stage_items "${_runtime_path}")
        endif()
    endforeach()
    unset(_runtime_path CACHE)
endif()
add_custom_command(TARGET coolbox_java_bridge POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_stage_items} "${COOLBOX_JAVA_NATIVE_OUTPUT_DIR}"
    COMMAND_EXPAND_LISTS
)
add_dependencies(coolbox_java_bridge coolbox_c_bindings)
