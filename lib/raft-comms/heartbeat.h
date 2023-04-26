//
// Created by zheraan on 19/04/23.
//

#ifndef THESIS_CSEC_HEARTBEAT_H
#define THESIS_CSEC_HEARTBEAT_H

#include <stdint.h>
#include <stdio.h>
#include "../hosts-list/host.h"
#include "../data-op.h"

typedef struct heartbeat_s {
    uint32_t host_id;
    enum host_status status;
    int flags;

    uint64_t next_index;
    uint64_t rep_index;
    uint64_t match_index;
    uint64_t commit_index;
    uint32_t term;
} heartbeat_s;

// Outputs the state of the structure to the specified output
void print_hb(heartbeat_s *hb, FILE *stream);

#endif //THESIS_CSEC_HEARTBEAT_H
