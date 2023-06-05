//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_RL_DATATYPES_H
#define THESIS_CSEC_RL_DATATYPES_H

#include <stdint.h>
#include "../mocked-fs/mfs-datatypes.h"

#ifndef LOG_LENGTH
#define LOG_LENGTH 2048 // Log length, in number of entries. May be redefined
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// Defines the entry's state
enum entry_state {
    ENTRY_STATE_INVALID = -2,
    ENTRY_STATE_EMPTY = -1,
    ENTRY_STATE_PROPOSAL = 0,
    ENTRY_STATE_PENDING = 1,
    ENTRY_STATE_COMMITTED = 2,
    ENTRY_STATE_CACHED = 3,
};

typedef struct log_entry_s {
    // P-Term the entry was proposed in
    uint32_t term;
    // State of the entry
    enum entry_state state;

    // Array of booleans to server hosts where it is replicated
    uint8_t *server_rep;
    // master hosts where it is replicated
    uint8_t *master_rep;
    // Is server replication majority achieved ?
    int server_maj;
    // Is master replication majority achieved ? Equals "Safe" state
    int master_maj;

    // Pointer to the data operation the entry refers to
    data_op_s op;
} log_entry_s;

typedef struct log_s {
    // Index of next log entry that will be added
    uint64_t next_index;
    // Index of latest log entry that is committed locally
    uint64_t commit_index;
    // Index of latest log entry that is committed by a majority of nodes
    uint64_t match_index;

    // current P-term
    uint32_t P_term;
    // current HS-term
    uint32_t HS_term;

    // Arrays of entries in the log. Defined as static array for performance.
    log_entry_s entries[LOG_LENGTH];
} log_s;

#endif // THESIS_CSEC_RL_DATATYPES_H
