//
// Created by zheraan on 27/04/23.
//

#ifndef THESIS_CSEC_OVERSEER_H
#define THESIS_CSEC_OVERSEER_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <event2/event.h>
#include <event2/util.h>

typedef struct overseer_s overseer_s;

#include "raft-comms/control-message.h"
#include "hosts-list/hosts-list.h"
#include "hosts-list/host.h"
#include "raft-entry-log/log.h"
#include "event-list.h"
#include "raft-comms/shared-events.h"
#include "mocked-fs/data-op.h"
#include "mocked-fs/mocked-fs.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

#ifndef ECFG_MAX_DISPATCH_USEC
#define ECFG_MAX_DISPATCH_USEC 2000
#endif

#ifndef ECFG_MAX_CALLBACKS
#define ECFG_MAX_CALLBACKS 5
#endif

// Program state structure.
// Contains pointers to its composing data structures and the communication socket
typedef struct overseer_s{
    struct event_base *eb;
    hosts_list_s *hl;
    log_s *log;
    evutil_socket_t socket_hb;
    evutil_socket_t socket_tr;
    event_list_s *el;
    mocked_fs_s *mfs;
}overseer_s;

// Initializes the program state structure passed by address.
// Allocates and fills its event base, hosts list, log and socket.
// Binds the socket to the local address.
// Self-cleans in case of failure and logs error source to stderr.
// Returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
int overseer_init(overseer_s *overseer);

// Allocates and configures a new event base.
// Returns a pointer to the newly allocated structure on success, NULL otherwise.
struct event_base *eb_new();

// Closes the communication socket and frees all components held in the program state
void overseer_wipe(overseer_s *overseer);

#endif //THESIS_CSEC_OVERSEER_H
