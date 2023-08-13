//
// Created by zheraan on 13/08/23.
//

#ifndef THESIS_CSEC_KILLER_H
#define THESIS_CSEC_KILLER_H

#include "../datatypes.h"

// Initializes the cluster monitor killer event, with a timeout that is randomized albeit constrained to not
// breach the program's invariants (at least half of the nodes of a certain type must be alive)
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int killer_init(overseer_s *overseer);

// Callback function for the killer events, sends a kill message to a random node
// Arg must be a pointer to the local overseer.
void killer_cb(evutil_socket_t fd, short event, void *arg);

// Sends a kill message to the target node, bypassing the fuzzer
// Shorthand for cm_sendto
int kill_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen);

// Determines if it is acceptable to kill a node by checking if there are enough other master nodes with the
// same next index as local, indicating that the cluster has recovered.
// Returns a boolean value based on the result.
int is_killing_ok(overseer_s *overseer);

#endif //THESIS_CSEC_KILLER_H
