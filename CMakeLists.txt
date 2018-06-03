cmake_minimum_required(VERSION 3.9)
project(c_noe_fork C)

set(CMAKE_C_STANDARD 99)

MESSAGE( STATUS "Starting build process")


#add_executable(main.exe src/test.c)
add_executable(gestore.exe src/gestore.c)
add_executable(individuo_a.exe src/individuo_a.c)
add_executable(individuo_b.exe src/individuo_b.c)
#add_executable(main.exe src/write_msg.c )
#add_executable(main.exe src/queue.c)
#add_executable(main.exe src/read_msg.c)
#add_executable(main.exe src/child_a_fifo.c)
#add_executable(main.exe src/shm_write.c)
#add_executable(main.exe src/shm_write.c)
#add_executable(main.exe src/shn_read.c)
#add_executable(main.exe src/struct_alloc.c)
#add_executable(main.exe src/alarm_loop.c)

#add_executable(main.exe src/write_shm.c)
#add_executable(main.exe src/read_shm.c)