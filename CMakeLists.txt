cmake_minimum_required(VERSION 4.2)
project(libgnss LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(LIBSERIALPORT_INCLUDE_DIR libserialport.h REQUIRED)
find_library(LIBSERIALPORT_LIBRARY NAMES serialport libserialport REQUIRED)

add_library(libgnss
  third_party/minmea/src/minmea.c
  src/nmea_reader.cpp
  src/rtcm.cpp
  src/gnss_receiver.cpp
  src/serial.cpp
)

target_compile_features(libgnss PUBLIC cxx_std_17)

target_include_directories(libgnss
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/minmea/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ringbuffer/include
    ${LIBSERIALPORT_INCLUDE_DIR}
)

target_link_libraries(libgnss PUBLIC ${LIBSERIALPORT_LIBRARY})
