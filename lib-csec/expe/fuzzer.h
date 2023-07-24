//
// Created by zheraan on 24/07/23.
//

#ifndef THESIS_CSEC_FUZZER_H
#define THESIS_CSEC_FUZZER_H

#include "expe-datatypes.h"
#include "../raft-comms/timeout.h"

// Creates and initializes a cache entry for the fuzzer storing the message's contents, which is sent later
// after a timeout. Essentially, it delays a message's transmission.
// If FUZZER_MESSAGE_DROPPING is true, uses the FUZZER_DROP_RATE as the rate at which messages are randomly
// dropped. When a message ends up being dropped, this function does nothing else.
// Returns EXIT_FAILURE if there was an issue with initialization, or EXIT_SUCCESS (even if a message is
// dropped).
int fuzzer_entry_init(overseer_s *overseer, enum packet_type t, union packet p, struct sockaddr_in6 addr,
                      socklen_t socklen);

// Frees all entries in the cache and their related events.
void fuzzer_cache_free_all(overseer_s *overseer);

// Frees the entry in the cache designated by the unique id given to it. Returns EXIT_FAILURE if no entry
// with the given id is found, or EXIT_SUCCESS otherwise.
int fuzzer_entry_free(overseer_s *overseer, uint32_t id);

// Callback function for the transmission of a packet after its fuzzer delay times out. arg is the cache entry
// containing that packet.
void fuzzer_transmission_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_FUZZER_H
