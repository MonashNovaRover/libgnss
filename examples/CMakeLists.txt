cmake_minimum_required(VERSION 3.16)

add_executable(reflex_test
  reflex_test.cpp
)

target_compile_features(reflex_test PRIVATE cxx_std_17)

target_link_libraries(reflex_test PRIVATE reflex)
