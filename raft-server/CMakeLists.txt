cmake_minimum_required(VERSION 3.22)
project(raft_server C)

set(CMAKE_C_STANDARD 17)

add_compile_definitions(HOST_TYPE=1) # Node is a server node
add_executable(raft_server main.c)