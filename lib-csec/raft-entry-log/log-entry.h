//
// Created by zheraan on 21/04/23.
//

#ifndef THESIS_CSEC_LOG_ENTRY_H
#define THESIS_CSEC_LOG_ENTRY_H

#include <stdint.h>
#include "../mocked-fs/data-op.h"
#include "../hosts-list/hosts-list.h"

enum entry_state{
    ENTRY_STATE_QUEUED = 0,
    ENTRY_STATE_PENDING = 1,
    ENTRY_STATE_COMMITTED = 2,
};

typedef struct log_entry_s {
    uint32_t term; // P-Term the entry was proposed in
    enum entry_state state;
    int is_cached;

    uint8_t *server_rep; // Array of booleans to server hosts where it is replicated
    uint8_t *master_rep; // master hosts where it is replicated
    int server_maj; // Is server replication majority achieved ?
    int master_maj; // Is master replication majority achieved ? Equals "Safe" state

    data_op_s op; // Pointer to the data operation the entry refers to
} log_entry_s;

#endif //THESIS_CSEC_LOG_ENTRY_H
