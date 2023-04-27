cmake_minimum_required(VERSION 3.25)
project(lib_csec C)

set(CMAKE_C_STANDARD 11)

add_library(
        lib_csec
        data-op.c
        shared-events.h
        data-op.h
        shared-events.c
        hosts-list/host.c
        hosts-list/host.h
        hosts-list/hosts-list.c
        hosts-list/hosts-list.h
        mocked-fs/mocked-fs.c
        mocked-fs/mocked-fs.h
        raft-comms/heartbeat.h
        raft-entry-log/log.c
        raft-entry-log/log.h
        raft-entry-log/log-entry.c
        raft-entry-log/log-entry.h
        raft-comms/heartbeat.c overseer.h)
