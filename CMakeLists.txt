cmake_minimum_required(VERSION 3.16)

project(libgnss VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBGNSS_BUILD_SHARED "Build libgnss as a shared library" OFF)

if(LIBGNSS_BUILD_SHARED)
	set(LIBGNSS_LIBRARY_TYPE SHARED)
else()
	set(LIBGNSS_LIBRARY_TYPE STATIC)
endif()

find_path(LIBSERIALPORT_INCLUDE_DIR serialport.h)
find_library(LIBSERIALPORT_LIBRARY serialport)

add_library(libgnss ${LIBGNSS_LIBRARY_TYPE}
	src/gnss_receiver.cpp
	src/nmea_reader.cpp
	src/rtcm_handler.cpp
	src/serial.cpp
	src/minmea.c
)

target_include_directories(libgnss
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(libgnss PUBLIC LIBGNSS_HAS_LIBSERIALPORT=1)
target_include_directories(libgnss PRIVATE ${LIBSERIALPORT_INCLUDE_DIR})
target_link_libraries(libgnss PRIVATE ${LIBSERIALPORT_LIBRARY})

target_compile_features(libgnss PUBLIC cxx_std_17)

