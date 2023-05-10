//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_RC_DATATYPES_H
#define THESIS_CSEC_RC_DATATYPES_H

#include <stdint.h>
#include "../hosts-list/hl-datatypes.h"
#include "../raft-log/rl-datatypes.h"
#include "../mocked-fs/data-op.h"

#define EUNKOWN_TIMEOUT_TYPE (-1)

#ifndef TIMEOUT_OFFSET_USEC
// minimum value of timeouts' timeval's usec value for randomized timers, maximum is offset + range
#define TIMEOUT_OFFSET_USEC 100
#endif

#ifndef TIMEOUT_OFFSET_SEC
// minimum value of timeouts' timeval's sec value for randomized timers, maximum is offset + range
#define TIMEOUT_OFFSET_SEC 0
#endif

// Defines for the range of the usec value in timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_USEC
#define TIMEOUT_RANGE_P_HB_USEC 0
#endif

#ifndef TIMEOUT_RANGE_HS_HB_USEC
#define TIMEOUT_RANGE_HS_HB_USEC 0
#endif

#ifndef TIMEOUT_RANGE_ELECTION_USEC
#define TIMEOUT_RANGE_ELECTION_USEC 150000 // Default value of 150ms
#endif

#ifndef TIMEOUT_RANGE_FUZZER_USEC
#define TIMEOUT_RANGE_FUZZER_USEC 0
#endif

#ifndef TIMEOUT_RANGE_RANDOM_OPS_USEC
#define TIMEOUT_RANGE_RANDOM_OPS_USEC 0
#endif

// Defines for the range of the sec value in timeouts' timeval
#ifndef TIMEOUT_RANGE_P_HB_SEC
#define TIMEOUT_RANGE_P_HB_SEC 1
#endif

#ifndef TIMEOUT_RANGE_HS_HB_SEC
#define TIMEOUT_RANGE_HS_HB_SEC 1
#endif

#ifndef TIMEOUT_RANGE_ELECTION_SEC
#define TIMEOUT_RANGE_ELECTION_SEC 0
#endif

#ifndef TIMEOUT_RANGE_FUZZER_SEC
#define TIMEOUT_RANGE_FUZZER_SEC 1
#endif

#ifndef TIMEOUT_RANGE_RANDOM_OPS_SEC
#define TIMEOUT_RANGE_RANDOM_OPS_SEC 8
#endif

// Defines for the fixed timeouts values
#ifndef TIMEOUT_VALUE_PROPOSITION_USEC
#define TIMEOUT_VALUE_PROPOSITION_USEC 500000 // Default value of 500ms
#endif

#ifndef TIMEOUT_VALUE_PROPOSITION_SEC
#define TIMEOUT_VALUE_PROPOSITION_SEC 0
#endif

#ifndef TIMEOUT_VALUE_ACK_SEC
#define TIMEOUT_VALUE_ACK_SEC 0
#endif

#ifndef TIMEOUT_VALUE_ACK_USEC
#define TIMEOUT_VALUE_ACK_USEC 100000 // Default value of 100ms
#endif

#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC 0
#endif

#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC 300000 // Default value of 300ms
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

enum timeout_type {
    // P master heartbeat timeout type
    TIMEOUT_TYPE_P_HB = 0,
    // HS master heartbeat timeout
    TIMEOUT_TYPE_HS_HB = 1,
    // Data op proposition timeout type, after which the proposition is deleted from queue
    TIMEOUT_TYPE_PROPOSITION = 2,
    // Message ack timeout type
    TIMEOUT_TYPE_ACK = 3,
    // Master election timeout type
    TIMEOUT_TYPE_ELECTION = 4,
    // Fuzzer delay introduction timeout type
    TIMEOUT_TYPE_FUZZER = 5,
    // Random data op generator timeout type
    TIMEOUT_TYPE_RANDOM_OPS = 6,
    // Timeout before retransmitting cached ops proposition
    TIMEOUT_TYPE_PROP_RETRANSMISSION = 7,
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

typedef struct transmission_s {
    // Status of the sender and type of transmission
    control_message_s cm;
    // Transmitted entry's related data op
    data_op_s op;
    // Transmitted entry's index
    uint32_t index;
    // Transmitted entry's term
    uint32_t term;
    // Transmitted entry's state
    enum entry_state state;
} transmission_s;

#endif //THESIS_CSEC_RC_DATATYPES_H
