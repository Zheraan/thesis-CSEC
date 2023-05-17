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

// Allocates a new transmission struct and initializes it with the parameters. The contents of the data op
// are copied in the transmission, so they can (and have to) be freed at some point afterward
entry_transmission_s *etr_new(const overseer_s *overseer,
                              enum message_type type,
                              const data_op_s *op,
                              uint32_t index,
                              uint32_t term,
                              enum entry_state state);

entry_transmission_s *etr_new_from_local_entry(const overseer_s *overseer,
                                               enum message_type type,
                                               uint64_t entry_id);

// Outputs the state of the structure to the specified stream
void etr_print(const entry_transmission_s *tr, FILE *stream);

// Sends a transmission to the specified address
// Returns either EXIT_SUCCESS or EXIT_FAILURE
int etr_sendto(overseer_s *overseer,
               struct sockaddr_in6 sockaddr,
               socklen_t socklen,
               entry_transmission_s *etr);

// Sends an entry transmission to the specified address, caches it for retransmission, and sets the callback
// event for it. The message sent will have the same ack_number as the retransmission cache id for that ETR.
// Returns either EXIT_SUCCESS or EXIT_FAILURE
int etr_sendto_with_rt_init(overseer_s *overseer,
                            entry_transmission_s *etr,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            enum message_type type,
                            uint8_t rt_attempts);

// Initializes the entry transmission reception event.
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int etr_reception_init(overseer_s *overseer);

// Callback for receiving transmissions. Processes the incoming transmission and resets the entry
// transmission event. Arg must be an overseer_s*
void etr_receive_cb(evutil_socket_t fd, short event, void *arg);

// Callback for the retransmission of ETRs. Arg must be a retransmission_s*. Cleans up the retransmission
// cache once maximum attempts have been reached
void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_ENTRY_TRANSMISSION_H
