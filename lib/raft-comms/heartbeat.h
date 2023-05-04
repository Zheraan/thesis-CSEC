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
#include "../mocked-fs/data-op.h"
#include "../overseer.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

#define HB_FLAG_DEFAULT 0b0
#define HB_FLAG_HB_ACK 0b1
#define HB_FLAG_P_TAKEOVER 0b10

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
void hb_print(heartbeat_s *hb, FILE *stream);

void heartbeat_receive_cb(evutil_socket_t fd, short event, void *arg);

// Sends an ack (heartbeat with ack flag) to the provided address with any eventual additional flags
void heartbeat_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, int flag);

heartbeat_s *heartbeat_new(overseer_s *overseer, int flags);

#endif //THESIS_CSEC_HEARTBEAT_H
