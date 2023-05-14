//
// Created by zheraan on 19/04/23.
//

#ifndef THESIS_CSEC_CONTROL_MESSAGE_H
#define THESIS_CSEC_CONTROL_MESSAGE_H

#include <stdio.h>
#include <event2/util.h>
#include <event2/event.h>
#include "rc-datatypes.h"
#include "../hosts-list/hosts-list.h"
#include "timeout.h"
#include "../overseer.h"

// Allocates a new control message struct and initializes it with the overseer's values. Returns NULL in case
// memory allocation fails.
// If message is of type INDICATE P, host id in the message is replaced by the id of the P node in the local
// host's hosts list. If no node has P status in this case, NULL is returned.
control_message_s *cm_new(const overseer_s *overseer, enum message_type type);

// Outputs the state of the structure to the specified stream
void cm_print(const control_message_s *hb, FILE *stream);

// Sends a control message to the provided address with the provided message type
int cm_sendto(const overseer_s *overseer,
              struct sockaddr_in6 sockaddr,
              socklen_t socklen,
              enum message_type type);

// Initializes the server-side control message reception event
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int cm_reception_init(overseer_s *overseer);

// Callback for receiving control messages
void cm_receive_cb(evutil_socket_t fd, short event, void *arg);

// Checks the metadata in the control message and returns a value indicating if any action needs to be taken.
// In case the message's term equals the local term, commits any uncommitted data that needs to be.
// Also updates the sender's status in the hosts-list.
enum cm_check_rv cm_check_metadata(overseer_s *overseer, const control_message_s *hb);

// Depending on the return value of cm_check_metadata(), takes the appropriate action
int cm_check_action(overseer_s *overseer, enum cm_check_rv rv);

// Initializes the retransmission event of a control message, caching the necessary data for doing so
int cm_retransmission_init(overseer_s *overseer,
                           struct sockaddr_in6 sockaddr,
                           socklen_t socklen,
                           enum message_type type,
                           uint8_t attempts);

// Callback for retransmitting events, cleans up after the maximum number of attempts has been made
void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_CONTROL_MESSAGE_H
