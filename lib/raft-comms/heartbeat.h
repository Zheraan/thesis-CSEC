//
// Created by zheraan on 19/04/23.
//

#ifndef THESIS_CSEC_HEARTBEAT_H
#define THESIS_CSEC_HEARTBEAT_H

#include <stdint.h>
#include <stdio.h>
#include <event2/util.h>
#include <event2/event.h>
#include "../hosts-list/host.h"
#include "../data-op.h"
#include "../overseer.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

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

extern int message_counter;

// Outputs the state of the structure to the specified output
void hb_print(heartbeat_s *hb, FILE *stream);

void heartbeat_sendto(evutil_socket_t sender, short event, void *arg);
void heartbeat_receive(evutil_socket_t listener, short event, void *arg);
heartbeat_s *heartbeat_new(overseer_s *overseer, int flags);

#endif //THESIS_CSEC_HEARTBEAT_H
