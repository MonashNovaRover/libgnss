cmake_minimum_required(VERSION 3.16)

add_executable(incremental_regex
  incremental_regex.cpp
)

target_link_libraries(incremental_regex PRIVATE
  reflex
  ringbuffer
)
