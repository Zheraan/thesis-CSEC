cmake_minimum_required(VERSION 3.22)
project(lib-csec C)

set(CMAKE_C_STANDARD 11)

add_library(
        lib-csec
        mocked-fs/data-op.h
        mocked-fs/data-op.c
        raft-comms/shared-events.h
        raft-comms/shared-events.c
        hosts-list/host.c
        hosts-list/host.h
        hosts-list/hosts-list.c
        hosts-list/hosts-list.h
        mocked-fs/mocked-fs.c
        mocked-fs/mocked-fs.h
        raft-comms/control-message.h
        raft-comms/control-message.c
        raft-entry-log/log.c
        raft-entry-log/log.h
        raft-entry-log/log-entry.c
        raft-entry-log/log-entry.h
        overseer.h
        overseer.c
        event-list.c
        event-list.h
        raft-comms/timeout.c
        raft-comms/timeout.h
        mocked-fs/ops-queue.c
        mocked-fs/ops-queue.h
        raft-comms/entry-transmission.c
        raft-comms/entry-transmission.h)
