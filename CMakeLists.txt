cmake_minimum_required(VERSION 3.5)
project(ex44 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(ex44 main.c
        osqueue.h osqueue.c threadPool.h threadPool.c)