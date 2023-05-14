//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_SERVER_EVENTS_H
#define THESIS_CSEC_SERVER_EVENTS_H

#include <event2/event.h>
#include "datatypes.h"
#include "raft-comms/retransmission-cache.h"
#include "raft-comms/control-message.h"
#include "raft-comms/timeout.h"
#include "raft-comms/entry-transmission.h"
#include "mocked-fs/ops-queue.h"
#include "hosts-list/hosts-list.h"
#include "overseer.h"

#ifndef QUEUE_ELEMENTS_TIMED_DELETION
// Enables the deletion of queued elements after a timeout to avoid stacking outdated elements if network is slow
#define QUEUE_ELEMENTS_TIMED_DELETION 1 // TODO Make timed deletion conditional
#endif

#ifndef PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS
// Defines how many times should the server attempt to retransmit a proposition before ditching it
#define PROPOSITION_RETRANSMISSION_MAX_ATTEMPTS 2 \
// TODO Label P is unreachable if too many attempts failed, or transition to partition mode
#endif

// Initializes the server-side random operation generation event, with a randomized timeout
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int server_random_ops_init(overseer_s *overseer);

// Callback function for the random ops events
// Attempts at creating a random op, and if successful, creates a request to the current P master
void server_random_ops_cb(evutil_socket_t fd, short event, void *arg);

// Deletes and frees all elements in the queue to prevent incoherences because
// subsequent operations might depend on the one that timed out
void server_proposition_dequeue_timeout_cb(evutil_socket_t fd, short event, void *arg);

// Creates a timeout event for deleting elements in the queue if timer has expired
// Returns EXIT_FAILURE in case of failure, requiring cleanup of target ops_queue element and subsequent ones
// Otherwise returns EXIT_SUCCESS
int server_queue_element_deletion_init(overseer_s *overseer, ops_queue_s *element);

// Caches related data op in case it is removed before retransmission (if needed), then sends it as a
// new log entry proposition to P and sets a timeout for retransmission
int server_proposition_transmit(overseer_s *overseer, ops_queue_s *element);

// Initializes the timer for retransmitting the cached proposition
int server_proposition_retransmission_init(overseer_s *overseer);

// Callback for retransmitting the cached proposition
void server_proposition_retransmission_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_SERVER_EVENTS_H
