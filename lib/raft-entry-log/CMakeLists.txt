cmake_minimum_required(VERSION 3.25)
project(raft_log C)

set(CMAKE_C_STANDARD 11)

add_library(raft_log log.c)
