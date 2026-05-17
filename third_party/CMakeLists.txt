cmake_minimum_required(VERSION 3.16)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(SIMDTestAndSetup)

set(reflex_sources
  debug.cpp
  error.cpp
  input.cpp
  matcher.cpp
  matcher_avx2.cpp
  matcher_avx512bw.cpp
  pattern.cpp
  simd.cpp
  simd_avx2.cpp
  simd_avx512bw.cpp
  utf8.cpp
)

list(TRANSFORM reflex_sources PREPEND "${CMAKE_CURRENT_LIST_DIR}/src/reflex/")

add_library(reflex STATIC
  ${reflex_sources}
)
target_compile_features(reflex PUBLIC cxx_std_11)
target_include_directories(reflex PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)
target_compile_definitions(reflex PRIVATE ${simd_definitions})
target_compile_options(reflex PRIVATE ${simd_flags})

set_target_properties(reflex PROPERTIES OUTPUT_NAME reflex)

if(WIN32)
  target_link_libraries(reflex PRIVATE ws2_32)
endif()
