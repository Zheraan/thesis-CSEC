#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include "rl-datatypes.h"
#include "../raft-comms/entry-transmission.h"

// Initializes the log's state
// Returns a pointer to the initialized log (same as argument)
log_s *log_init(log_s *log);

// Adds the entry contained in the transmission to the log as new entry with the given state.
// State parameter is voluntarily redundant with transmission's state field to allow for caching new entries
// in a log that has missing entries, to not require them again when repairing the missing ones.
// If the state parameter is ENTRY_STATE_CACHED, log indexes are not modified when adding the new entry.
// Returns EXIT_FAILURE in case of failure (ie. log is full) or EXIT_SUCCESS otherwise.
int log_add_entry(overseer_s *overseer, const entry_transmission_s *tr, enum entry_state state);

// Frees the log entire log stored in the pointer, as well as its entries' replication arrays
void log_free(log_s *log);

// Frees the replication arrays in the log entry but not the entry itself
void log_entry_replication_arrays_free(log_entry_s *entry);

// Returns a pointer to the entry with the given id, or NULL if there is none
log_entry_s *log_get_entry_by_id(log_s *log, uint64_t id);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
