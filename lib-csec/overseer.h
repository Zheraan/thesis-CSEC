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

#include "datatypes.h"
#include "hosts-list/hosts-list.h"
#include "raft-log/log.h"
#include "mocked-fs/ops-queue.h"
#include "event-list.h"

#ifndef ECFG_MAX_DISPATCH_USEC
#define ECFG_MAX_DISPATCH_USEC 2000
#endif

#ifndef ECFG_MAX_CALLBACKS
#define ECFG_MAX_CALLBACKS 5
#endif

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
