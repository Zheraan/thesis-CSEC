cmake_minimum_required(VERSION 3.22)
project(raft_server C)

set(CMAKE_C_STANDARD 17)

add_executable(raft_server main.c server-events.c server-events.h)

target_link_libraries(raft_server lib_csec)