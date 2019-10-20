cmake_minimum_required(VERSION 3.0)
project(ftp_server)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(SOURCE_FILES src/server.c include/user.h src/user.cpp)

include_directories(include)

add_executable(server ${SOURCE_FILES})

