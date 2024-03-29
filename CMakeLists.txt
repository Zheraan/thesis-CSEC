cmake_minimum_required(VERSION 3.22)
project(thesis_csec C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4 -Wall -Wextra -Wvla -Wno-unused-parameter")
set(CMAKE_C_STANDARD 17)

add_library(libevent_core STATIC IMPORTED)
set_target_properties(libevent_core PROPERTIES IMPORTED_LOCATION /usr/local/lib/libevent_core.a)
set_target_properties(libevent_core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES /usr/local/include)

add_library(libevent_extra STATIC IMPORTED)
set_target_properties(libevent_extra PROPERTIES IMPORTED_LOCATION /usr/local/lib/libevent_extra.a)
set_target_properties(libevent_extra PROPERTIES INTERFACE_INCLUDE_DIRECTORIES /usr/local/include)

add_subdirectory(lib-csec)
include_directories(lib-csec)

add_subdirectory(raft-server)
target_link_libraries(raft_server lib-csec libevent_core libevent_extra)

add_subdirectory(raft-master)
target_link_libraries(raft_master lib-csec libevent_core libevent_extra)

add_subdirectory(cluster-monitor)
target_link_libraries(cluster_monitor lib-csec libevent_core libevent_extra)
