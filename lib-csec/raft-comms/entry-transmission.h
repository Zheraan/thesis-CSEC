//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_ENTRY_TRANSMISSION_H
#define THESIS_CSEC_ENTRY_TRANSMISSION_H

#include <stdlib.h>
#include <stdint.h>
#include "rc-datatypes.h"
#include "../raft-log/log.h"
#include "control-message.h"

// Outputs the state of the structure to the specified stream
void etr_print(const entry_transmission_s *tr, FILE *stream);

// Sends a transmission to the specified address
int etr_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, entry_transmission_s *tr);

// Callback for receiving transmissions
void etr_receive_cb(evutil_socket_t fd, short event, void *arg);

// Allocates a new transmission struct and initializes it with the parameters. The contents of the data op
// are copied in the transmission, so they can (and have to) be freed at some point afterward
entry_transmission_s *etr_new(const overseer_s *overseer,
                              enum message_type type,
                              const data_op_s *op,
                              uint32_t index,
                              uint32_t term,
                              enum entry_state state);


void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg);

entry_transmission_s *etr_new_from_local_entry(const overseer_s *overseer,
                                               enum message_type type,
                                               uint64_t entry_id);

int etr_sendto_with_rt_init(overseer_s *overseer,
                            entry_transmission_s *etr,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            enum message_type type,
                            uint8_t attempts);

#endif //THESIS_CSEC_ENTRY_TRANSMISSION_H
