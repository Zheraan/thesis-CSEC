//
// Created by zheraan on 21/04/23.
//

#ifndef THESIS_CSEC_LOG_ENTRY_H
#define THESIS_CSEC_LOG_ENTRY_H

#ifndef HOSTS_LIST_SIZE
#define HOSTS_LIST_SIZE 128
#endif

#define ENTRY_STATE_CREATED 1
#define ENTRY_STATE_QUEUED 2
#define ENTRY_STATE_PENDING 3
#define ENTRY_STATE_COMMITTED 4

#include "stdint.h"
#include "../data-op.h"

/* Log Entry Data structure
 * - term of adoption
 * - state (created, queued, pending, committed)
 * - server replication array
 * - master replication array
 * - server majority replication reached
 * - master majority replication reached
 * - data operation
 */
typedef struct log_entry{
    uint64_t adoption_term;
    uint8_t state;
    uint16_t server_replication[HOSTS_LIST_SIZE];
    uint16_t master_replication[HOSTS_LIST_SIZE];
    bool server_majority;
    bool master_majority; // == "Safe"
    data_op op;
} log_entry;

#endif //THESIS_CSEC_LOG_ENTRY_H
