cmake_minimum_required(VERSION 3.25)
project(thesis_csec C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(lib)
include_directories(lib)

add_subdirectory(raft-server)
target_link_libraries(raft_server lib_csec)

add_subdirectory(raft-master)
target_link_libraries(raft_master lib_csec)
