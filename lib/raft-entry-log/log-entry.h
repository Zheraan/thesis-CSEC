//
// Created by zheraan on 21/04/23.
//

#ifndef THESIS_CSEC_LOG_ENTRY_H
#define THESIS_CSEC_LOG_ENTRY_H

#define ENTRY_STATE_CREATED 1
#define ENTRY_STATE_QUEUED 2
#define ENTRY_STATE_PENDING 3
#define ENTRY_STATE_COMMITTED 4

#include <stdint.h>
#include "../data-op.h"
#include "../hosts-list/hosts-list.h"

typedef struct log_entry_s {
    uint32_t term; // P-Term the entry was proposed in
    uint8_t state; // Refer to the ENTRY_STATE_* macros

    uint16_t server_rep[HOSTS_LIST_SIZE]; // ID of server hosts where it is replicated
    uint16_t master_rep[HOSTS_LIST_SIZE]; // ID of master hosts where it is replicated
    int server_maj; // Is server replication majority achieved ?
    int master_maj; // Is master replication majority achieved ? Equals "Safe" state

    data_op_s *op; // Pointer to the data operation the entry refers to
} log_entry_s;

#endif //THESIS_CSEC_LOG_ENTRY_H
