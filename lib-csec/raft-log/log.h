#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include "rl-datatypes.h"
#include "../raft-comms/entry-transmission.h"

// Initializes the log's state
// Returns a pointer to the initialized log (same as argument)
log_s *log_init(log_s *log);

// Adds the entry contained in the transmission to the log as new entry with the given state.
// State parameter is voluntarily redundant with transmission's state field to allow for better control.
// Returns EXIT_FAILURE in case of failure (ie. log is full) or EXIT_SUCCESS otherwise.
int log_add_entry(overseer_s *overseer, const transmission_s *tr, enum entry_state state);

void log_free(log_s *log);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
