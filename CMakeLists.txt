cmake_minimum_required(VERSION 3.9)
project(c_noe_fork C)

set(CMAKE_C_STANDARD 99)

MESSAGE( STATUS "Starting build process")


#add_executable(main.exe src/test.c)
#add_executable(main.exe src/main.c)
add_executable(main.exe src/alarm_loop.c)