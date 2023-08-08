cmake_minimum_required(VERSION 3.22)
project(cluster_monitor C)

set(CMAKE_C_STANDARD 17)

add_compile_definitions(HOST_TYPE=2) # Node is a master node
add_executable(cluster_monitor main.c)
