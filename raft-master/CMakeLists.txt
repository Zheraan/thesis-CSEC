cmake_minimum_required(VERSION 3.25)
project(raft_master C)

set(CMAKE_C_STANDARD 17)

add_executable(raft_master main.c master-events.c master-events.h)
