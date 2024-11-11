cmake_minimum_required(VERSION 3.29)

project(my_shell C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")

file(GLOB SOURCES "my_shell.c" "src/*.c")

add_executable(my_shell ${SOURCES})
