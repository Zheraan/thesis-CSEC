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
#include "../overseer.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

// Outputs the state of the structure to the specified stream
void cm_print(const control_message_s *hb, FILE *stream);

// Sends a control message to the provided address with the provided message type
void cm_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type);

// Callback for receiving control messages
void cm_receive_cb(evutil_socket_t fd, short event, void *arg);

// Allocates a new control message struct and initializes it with the overseer's values. Returns NULL in case
// memory allocation fails.
// If message is of type INDICATE P, host id in the message is replaced by the id of the P node in the local
// host's hosts list. If no node has P status in this case, NULL is returned.
control_message_s *cm_new(const overseer_s *overseer, enum message_type type);

#endif //THESIS_CSEC_CONTROL_MESSAGE_H
