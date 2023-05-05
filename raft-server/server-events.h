//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_SERVER_EVENTS_H
#define THESIS_CSEC_SERVER_EVENTS_H

#include <event2/event.h>
#include "overseer.h"
#include "raft-comms/heartbeat.h"
#include "timeout.h"

// Initializes the server-side heartbeat reception event
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int server_reception_init(overseer_s *overseer);

// Initializes the server-side random operation generation event, with a randomized timeout
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int server_random_ops_init(overseer_s *overseer);

// Callback function for the random ops events
// Attempts at creating a random op, and if successful, creates a request to the current P master
void server_random_ops_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_SERVER_EVENTS_H
