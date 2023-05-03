cmake_minimum_required(VERSION 3.22)
project(lib_csec C)

set(CMAKE_C_STANDARD 11)

add_library(
        lib_csec
        mocked-fs/data-op.c
        mocked-fs/data-op.h
        shared-events.h
        shared-events.c
        hosts-list/host.c
        hosts-list/host.h
        hosts-list/hosts-list.c
        hosts-list/hosts-list.h
        mocked-fs/mocked-fs.c
        mocked-fs/mocked-fs.h
        raft-comms/heartbeat.h
        raft-comms/heartbeat.c
        raft-entry-log/log.c
        raft-entry-log/log.h
        raft-entry-log/log-entry.c
        raft-entry-log/log-entry.h
        overseer.h
        overseer.c
        event-list.c
        event-list.h)
