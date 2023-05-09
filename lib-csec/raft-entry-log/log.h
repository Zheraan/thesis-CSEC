#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include <stdint.h>
#include "log-entry.h"
#include "../raft-comms/entry-transmission.h"

#ifndef LOG_LENGTH
#define LOG_LENGTH 2048 // Log length, in number of entries. May be redefined
#endif

typedef struct log_s {
    uint64_t next_index; // index of next log entry that will be added
    uint64_t rep_index; // index of latest log entry that is either pending, committed or applied
    uint64_t match_index; // index of last log entry replicated by a majority of nodes

    uint32_t P_term; // current P-term
    uint32_t HS_term; // current HS-term

    log_entry_s entries[LOG_LENGTH]; // Arrays of entries in the log. Defined as static array for performance.
} log_s;

// Initializes the log's state
// Returns a pointer to the initialized log (same as argument)
log_s *log_init(log_s *log);

// Adds the entry contained in the transmission to the log as new entry with the given state.
// State parameter is voluntarily redundant with transmission's state field to allow for better control.
// Returns EXIT_FAILURE in case of failure (ie. log is full) or EXIT_SUCCESS otherwise.
int log_add_entry(overseer_s *overseer, const transmission_s *tr, enum entry_state state);

#endif //RAFT_ENTRY_LOG_LIBRARY_H
