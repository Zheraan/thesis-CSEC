//
// Created by zheraan on 06/08/23.
//

#ifndef THESIS_CSEC_STATE_MONITORING_H
#define THESIS_CSEC_STATE_MONITORING_H

#include "expe-datatypes.h"

// Allocates a new program state transmission struct and initializes it with the current state.
// Returns NULL if the allocation fails, or a pointer to the newly allocated struct.
program_state_transmission_s *pstr_new(overseer_s *overseer);

// Prints the contents ofr the PSTR to the given stream.
// The flags parameter may be CSEC_FLAG_DEFAULT (zero) or FLAG_PRINT_SHORT for a shorter inline print
void pstr_print(program_state_transmission_s *pstr, FILE *stream, int flags);

// Creates an image of the program state and sends it to all cluster monitor nodes using the CM port, as
// it is not necessary for cluster monitors to receive CMs.
// Returns EXIT_SUCCESS, or EXIT_FAILURE if at least one transmission failed
int pstr_transmit(overseer_s *overseer);

// Initializes the PSTR reception event.
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int pstr_reception_init(overseer_s *overseer);

// Callback for receiving PSTRs. Processes the incoming transmission and resets the PSTR reception event.
// Arg must be a pointer to the local overseer
void pstr_receive_cb(evutil_socket_t fd, short event, void *arg);

// Checks the
uint64_t pstr_actions(overseer_s *overseer, program_state_transmission_s *pstr);

#endif //THESIS_CSEC_STATE_MONITORING_H
