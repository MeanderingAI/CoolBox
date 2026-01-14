add_library(distributed
    message_passing.cpp
    distributed_trainer.cpp
)

target_include_directories(distributed PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(distributed
    ${CMAKE_THREAD_LIBS_INIT}
)
