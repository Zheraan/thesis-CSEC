//
// Created by zheraan on 19/04/23.
//

#ifndef THESIS_CSEC_CONTROL_MESSAGE_H
#define THESIS_CSEC_CONTROL_MESSAGE_H

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

// Control message type values, including those only used within the transmission struct
enum message_type {
    // Default heartbeat
    MSG_TYPE_HB_DEFAULT = 0,
    // Ack of heartbeat without issues
    MSG_TYPE_ACK_HB = 1,
    // New P taking over
    MSG_TYPE_P_TAKEOVER = 2,
    // New HS taking over
    MSG_TYPE_HS_TAKEOVER = 3,
    // Repairing the log, asking master for entry before next index
    MSG_TYPE_LOG_REPAIR = 4,
    // Replaying the log, asking master for entry in next index
    MSG_TYPE_LOG_REPLAY = 5,
    // Master sending a new pending entry (only valid within transmissions)
    MSG_TYPE_TR_ENTRY = 6,
    // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_TR_COMMIT = 7,
    // Server sending a new entry proposition to P (only valid within transmissions)
    MSG_TYPE_TR_PROPOSITION = 8,
    // Master sending a new pending entry (only valid within transmissions)
    MSG_TYPE_ACK_ENTRY = 9,
    // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_ACK_COMMIT = 10,
    // Message to signify a master with outdated term to stand down from its role
    MSG_TYPE_DEMOTE_NOTICE = 11,
    // Message for indicating who is P in case host that is not P received a message sent to P
    MSG_TYPE_INDICATE_P = 12,
};

typedef struct control_message_s {
    // Message sender's id in the hosts list
    uint32_t host_id;
    // Status of the sender
    enum host_status status;
    // Enum for determining the type of control message
    enum message_type type;

    // Sender's next index
    uint64_t next_index;
    // Sender's replication index (highest index that is either committed or pending)
    uint64_t rep_index;
    // Sender's match index
    uint64_t match_index;
    // Sender's current P-term
    uint32_t term;
} control_message_s;

// Outputs the state of the structure to the specified stream
void cm_print(const control_message_s *hb, FILE *stream);

// Sends a control message to the provided address with the provided message type
void cm_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type);

// Callback for receiving control messages
void cm_receive_cb(evutil_socket_t fd, short event, void *arg);

// Allocates a new control message struct and initializes it with the overseer's values. Returns NULL in case
// memory allocation fails.
// If message is of type INDICATE P, host id in the message is replaced by the id of the P node in the local
// host's hosts list. If no node has P status in this case, NULL is returned.
control_message_s *cm_new(const overseer_s *overseer, enum message_type type);

#endif //THESIS_CSEC_CONTROL_MESSAGE_H
