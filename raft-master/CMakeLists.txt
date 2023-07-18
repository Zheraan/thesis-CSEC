cmake_minimum_required(VERSION 3.22)
project(raft_master C)

set(CMAKE_C_STANDARD 17)

add_compile_definitions(HOST_TYPE=0) # Node is a master node
add_executable(raft_master main.c)
