//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_RL_DATATYPES_H
#define THESIS_CSEC_RL_DATATYPES_H

#include <stdint.h>
#include "../raft-comms/rc-datatypes.h"
#include "../mocked-fs/mfs-datatypes.h"

#ifndef LOG_LENGTH
#define LOG_LENGTH 2048 // Log length, in number of entries. May be redefined
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#ifndef ENTRY_STATE_
#define ENTRY_STATE_
// Defines the entry's state
enum entry_state {
    ENTRY_STATE_INVALID = -1,
    ENTRY_STATE_EMPTY = 0,
    ENTRY_STATE_PROPOSAL = 1,
    ENTRY_STATE_PENDING = 2,
    ENTRY_STATE_COMMITTED = 3,
    ENTRY_STATE_CACHED = 4,
};
#endif

enum fix_type {
    FIX_TYPE_NONE = 0,
    FIX_TYPE_REPAIR = 1,
    FIX_TYPE_REPLAY = 2,
};

typedef struct log_entry_s {
    // P-Term the entry was proposed in
    uint32_t term;
    // State of the entry
    enum entry_state state;

    // Number of servers that have replicated this entry
    uint32_t server_rep;
    // Number of masters that have replicated this entry
    uint32_t master_rep;

    // Pointer to the data operation the entry refers to
    data_op_s op;
} log_entry_s;

typedef struct log_s {
    // Index of next log entry that will be added
    uint64_t next_index;
    // Index of latest log entry that is committed locally
    uint64_t commit_index;

    // current P-term
    uint32_t P_term;
    // current HS-term
    uint32_t HS_term;

    // Number of server nodes needed to reach majority
    uint32_t server_majority;
    // Number of master nodes needed to reach majority
    uint32_t master_majority;

    // Contains the ID of the latest retransmission-cache entry related to an ongoing log repair or replay
    // conversation, or 0 otherwise.
    uint32_t fix_conversation;
    // Contains the type of ongoing fix, or FIX_TYPE_NONE otherwise.
    enum fix_type fix_type;

    // Arrays of entries in the log. Defined as static array for performance.
    log_entry_s entries[LOG_LENGTH];
} log_s;

#endif // THESIS_CSEC_RL_DATATYPES_H
