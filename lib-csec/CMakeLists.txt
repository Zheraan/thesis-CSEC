cmake_minimum_required(VERSION 3.22)
project(lib-csec C)

set(CMAKE_C_STANDARD 11)

add_library(
        lib-csec
        lib-csec.h
        datatypes.h
        hosts-list/hl-datatypes.h
        mocked-fs/mfs-datatypes.h
        raft-comms/rc-datatypes.h
        raft-log/rl-datatypes.h
        mocked-fs/data-op.c
        mocked-fs/data-op.h
        hosts-list/hosts-list.c
        hosts-list/hosts-list.h
        mocked-fs/mocked-fs.c
        mocked-fs/mocked-fs.h
        raft-comms/control-message.c
        raft-comms/control-message.h
        raft-log/log.c
        raft-log/log.h
        overseer.c
        overseer.h
        raft-comms/retransmission-cache.c
        raft-comms/retransmission-cache.h
        raft-comms/timeout.c
        raft-comms/timeout.h
        mocked-fs/ops-queue.c
        mocked-fs/ops-queue.h
        raft-comms/entry-transmission.c
        raft-comms/entry-transmission.h
        status/p-liveness.c
        status/p-liveness.h
        status/elections.c
        status/elections.h
        status/master-events.c
        status/master-events.h
        status/server-events.c
        status/server-events.h
        status/status-datatypes.h)
