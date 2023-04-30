//
// Created by zheraan on 27/04/23.
//

#ifndef THESIS_CSEC_OVERSEER_H
#define THESIS_CSEC_OVERSEER_H

#include <event2/event.h>
#include <unistd.h>
#include "hosts-list/hosts-list.h"
#include "raft-entry-log/log.h"
#include "shared-events.h"

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
    evutil_socket_t *udp_socket;
}overseer_s;

// Initializes the program state structure passed by address.
// Allocates and fills its event base, hosts list, log and socket.
// Returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
int overseer_init(overseer_s *overseer);

// Allocates and configures a new event base.
// Returns a pointer to the newly allocated structure on success, NULL otherwise.
struct event_base *eb_new();

// Closes the communication socket and frees all components held in the program state
void overseer_wipe(overseer_s *overseer);

#endif //THESIS_CSEC_OVERSEER_H
