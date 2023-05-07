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

enum message_type { // TODO Implement all those transmissions
    MSG_TYPE_HB_DEFAULT = 0, // Default heartbeat
    MSG_TYPE_ACK_HB = 1, // Ack of heartbeat without issues
    MSG_TYPE_P_TAKEOVER = 2, // New P taking over
    MSG_TYPE_HS_TAKEOVER = 3, // New HS taking over
    MSG_TYPE_REPAIR = 4, // Repairing the log, asking master for entry before next index
    MSG_TYPE_REPLAY = 5, // Replaying the log, asking master for entry in next index
    MSG_TYPE_TR_ENTRY = 6, // Master sending a new pending entry (only valid within transmissions)
    MSG_TYPE_TR_COMMIT = 7, // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_TR_PROPOSITION = 8, // Server sending a new entry proposition (only valid within transmissions)
    MSG_TYPE_ACK_ENTRY = 9, // Master sending a new pending entry (only valid within transmissions)
    MSG_TYPE_ACK_COMMIT = 10, // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_DEMOTE = 11, // Message to signify a master with outdated term to stand down from its role
};

typedef struct heartbeat_s {
    uint32_t host_id; // Heartbeat sender's id in the hosts list
    enum host_status status; // Status of the sender
    enum message_type type; // Flags for determining the type of heartbeat

    uint64_t next_index; // Sender's next index
    uint64_t rep_index; // Sender's replication index (highest index that is either committed or pending)
    uint64_t match_index; // Sender's match index
    uint32_t term; // Sender's current P-term
} heartbeat_s;

// Outputs the state of the structure to the specified output
void hb_print(heartbeat_s *hb, FILE *stream);

// Callback for receiving heartbeat messages
void heartbeat_receive_cb(evutil_socket_t fd, short event, void *arg);

// Sends an ack (heartbeat with ack flag) to the provided address with any eventual additional flags
void heartbeat_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type);

heartbeat_s *heartbeat_new(overseer_s *overseer, enum message_type type);

#endif //THESIS_CSEC_HEARTBEAT_H
