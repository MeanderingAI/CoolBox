add_library(distributed SHARED
    source/distributed_trainer.cpp
    source/message_passing.cpp
)
target_include_directories(distributed PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
