#ifndef RAFT_ENTRY_LOG_LIBRARY_H
#define RAFT_ENTRY_LOG_LIBRARY_H

#include <stdint.h>
#include "log-entry.h"

#ifndef LOG_LENGTH
#define LOG_LENGTH 2048 // Log length, in number of entries. May be redefined
#endif

typedef struct log_s {
    uint64_t next_index; // index of next log entry that will be added
    uint64_t rep_index; // index of latest log entry that is either pending, committed or applied
    uint64_t match_index; // index of last log entry replicated by all nodes
    uint64_t commit_index; // index of last log committed locally

    uint32_t P_term; // current P-term
    uint32_t HS_term; // current HS-term

    log_entry_s entries[LOG_LENGTH]; // Arrays of entries in the log. Defined as static array for performance.

    // TODO snapshot version
} log_s;

#endif //RAFT_ENTRY_LOG_LIBRARY_H
