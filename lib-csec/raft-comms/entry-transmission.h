//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_ENTRY_TRANSMISSION_H
#define THESIS_CSEC_ENTRY_TRANSMISSION_H

#include <stdlib.h>
#include <stdint.h>
#include "control-message.h"

typedef struct transmission_s {
    control_message_s cm; // Status of the sender and type of transmission
    data_op_s op; // Transmitted entry's related data op
    uint32_t index; // Transmitted entry's index
    uint32_t term; // Transmitted entry's term
    enum entry_state state; // Transmitted entry's term
} transmission_s;

// Outputs the state of the structure to the specified stream
void tr_print(const transmission_s *tr, FILE *stream);

// Sends a transmission to the specified address
int tr_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, transmission_s *tr);

// Callback for receiving transmissions
void tr_receive_cb(evutil_socket_t fd, short event, void *arg);

// Allocates a new transmission struct and initializes it with the parameters
transmission_s *tr_new(const overseer_s *overseer,
                       enum message_type type,
                       const data_op_s *op,
                       uint32_t index,
                       uint32_t term,
                       enum entry_state state);

#endif //THESIS_CSEC_ENTRY_TRANSMISSION_H
