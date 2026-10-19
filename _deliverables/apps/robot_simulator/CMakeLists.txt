include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
FetchContent_Declare(robot_raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(robot_raylib)

set(BUILD_BULLET3 OFF CACHE BOOL "Build Bullet 3 compute libraries" FORCE)
set(BUILD_ENET OFF CACHE BOOL "Build Bullet UDP networking" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "Build Bullet TCP networking" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "Build Bullet demos" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "Build Bullet CPU demos" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "Build Bullet OpenGL demos" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Build Bullet unit tests" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "Build Bullet extras" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "Install Bullet libraries" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(robot_bullet
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG 3.25
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(robot_bullet)

add_executable(robot_simulator src/main.cpp)
target_compile_features(robot_simulator PRIVATE cxx_std_17)
target_include_directories(robot_simulator SYSTEM PRIVATE ${robot_bullet_SOURCE_DIR}/src)
target_link_libraries(robot_simulator PRIVATE raylib BulletDynamics BulletCollision LinearMath robot_navigation)
if(MSVC)
    target_compile_definitions(robot_simulator PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

if(COMMAND coolbox_stage_test_runtime_dependencies)
    coolbox_stage_test_runtime_dependencies(robot_simulator)
endif()