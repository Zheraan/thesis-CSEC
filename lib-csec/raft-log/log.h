#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include "rl-datatypes.h"
#include "../raft-comms/entry-transmission.h"

// Initializes the log's state
// Returns a pointer to the initialized log (same as argument)
log_s *log_init(log_s *log);

void log_print(log_s *log); //TODO

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

int log_repair_start(overseer_s *overseer); // TODO implement log_repair_start

int log_replay_start(overseer_s *overseer); // TODO implement log_replay_start

int log_repair_ongoing(overseer_s *overseer); // TODO implement log_repair_ongoing

int log_replay_ongoing(overseer_s *overseer); // TODO implement log_replay_ongoing

// Marks all non-empty entries from given index included as invalid, and sets the log's next index as the
// given index if it was greater than it.
void log_invalidate_from(log_s *log, uint64_t index);

// Commits the log entry with the given index and applies it in the MFS, may fail if entry is marked
// as empty or invalid, or if the commit index is not equal to given index minus 1
int log_entry_commit(overseer_s *overseer, uint64_t index);

// Commit log entries from the local commit index up to the given index (included) and applies them in the MFS.
int log_commit_upto(overseer_s *overseer, uint64_t index);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
