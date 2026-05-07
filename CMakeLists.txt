cmake_minimum_required(VERSION 4.2)
project(libgnss)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(libgnss
        include/minmea.h
        include/nmea_reader.hpp
        include/ringbuffer.hpp
        src/minmea.c
        src/nmea_reader.cpp
        include/rtcm_handler.hpp
        src/rtcm_handler.cpp)
