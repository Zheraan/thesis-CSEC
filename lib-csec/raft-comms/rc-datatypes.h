//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_RC_DATATYPES_H
#define THESIS_CSEC_RC_DATATYPES_H

#include <stdint.h>
#include "../hosts-list/hl-datatypes.h"
#include "../raft-log/rl-datatypes.h"
#include "../mocked-fs/data-op.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// Error code for unknown timeout type
#define EUNKNOWN_TIMEOUT_TYPE (-1)

// Timeout duration range for elections. Default value of 500ms
#ifndef TIMEOUT_RANGE_ELECTION_USEC
#define TIMEOUT_RANGE_ELECTION_USEC 500000
#endif
#ifndef TIMEOUT_RANGE_ELECTION_SEC
#define TIMEOUT_RANGE_ELECTION_SEC 0
#endif
// Timeout duration offset for elections. Default value of 150ms
#ifndef TIMEOUT_OFFSET_ELECTION_USEC
#define TIMEOUT_OFFSET_ELECTION_USEC 150000
#endif
#ifndef TIMEOUT_OFFSET_ELECTION_SEC
#define TIMEOUT_OFFSET_ELECTION_SEC 0
#endif

// Timeout duration range for the fuzzer. Default value of 2s
#ifndef TIMEOUT_RANGE_FUZZER_USEC
#define TIMEOUT_RANGE_FUZZER_USEC 999999
#endif
#ifndef TIMEOUT_RANGE_FUZZER_SEC
#define TIMEOUT_RANGE_FUZZER_SEC 1
#endif
// Timeout duration offset for the fuzzer. Default value of 10ms
#ifndef TIMEOUT_OFFSET_FUZZER_SEC
#define TIMEOUT_OFFSET_FUZZER_SEC 0
#endif
#ifndef TIMEOUT_OFFSET_FUZZER_USEC
#define TIMEOUT_OFFSET_FUZZER_USEC 10000
#endif

// Timeout duration range for the random op generator. Default value of 8s
#ifndef TIMEOUT_RANGE_RANDOM_OPS_USEC
#define TIMEOUT_RANGE_RANDOM_OPS_USEC 999999
#endif
#ifndef TIMEOUT_RANGE_RANDOM_OPS_SEC
#define TIMEOUT_RANGE_RANDOM_OPS_SEC 7
#endif
// Timeout duration offset for the random op generator. Default value of 0ms
#ifndef TIMEOUT_OFFSET_RANDOM_OPS_SEC
#define TIMEOUT_OFFSET_RANDOM_OPS_SEC 0
#endif
#ifndef TIMEOUT_OFFSET_RANDOM_OPS_USEC
#define TIMEOUT_OFFSET_RANDOM_OPS_USEC 0
#endif

// Timeout duration for propositions. Default value of 500ms
#ifndef TIMEOUT_VALUE_PROPOSITION_USEC
#define TIMEOUT_VALUE_PROPOSITION_USEC 500000
#endif
#ifndef TIMEOUT_VALUE_PROPOSITION_SEC
#define TIMEOUT_VALUE_PROPOSITION_SEC 0
#endif

// Timeout duration for Acks. Default value of 150ms
#ifndef TIMEOUT_VALUE_ACK_SEC
#define TIMEOUT_VALUE_ACK_SEC 0
#endif
#ifndef TIMEOUT_VALUE_ACK_USEC
#define TIMEOUT_VALUE_ACK_USEC 150000
#endif

// Timeout duration for the proposition retransmission. Default value of 300ms
#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC 0
#endif
#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC 300000
#endif

// Timeout duration for P's heartbeat. Default value of 2s
#ifndef TIMEOUT_VALUE_P_HB_SEC
#define TIMEOUT_VALUE_P_HB_SEC 2
#endif
#ifndef TIMEOUT_VALUE_P_HB_USEC
#define TIMEOUT_VALUE_P_HB_USEC 0
#endif

// Timeout duration for HS's heartbeat. Default value of 2s
#ifndef TIMEOUT_VALUE_HS_HB_SEC
#define TIMEOUT_VALUE_HS_HB_SEC 2
#endif
#ifndef TIMEOUT_VALUE_HS_HB_USEC
#define TIMEOUT_VALUE_HS_HB_USEC 0
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
    // Master sending a new entry (only valid within transmissions)
    MSG_TYPE_TR_ENTRY = 6,
    // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_TR_COMMIT = 7,
    // Server sending a new entry proposition to P (only valid within transmissions)
    MSG_TYPE_TR_PROPOSITION = 8,
    // Master sending a new pending entry (only valid within transmissions)
    MSG_TYPE_ACK_ENTRY = 9,
    // Master sending the commit order for an entry (only valid within transmissions)
    MSG_TYPE_ACK_COMMIT = 10,
    // Message for indicating who is P in case host that is not P received a message sent to P or if the sender
    // of a message had an outdated P-term
    MSG_TYPE_INDICATE_P = 11,
    // Message for indicating who is HS in case host that is not HS received a message sent to HS or if the sender
    // of a message had an outdated HS-term
    MSG_TYPE_INDICATE_HS = 12,
};

// Control message metadata check return values specifying the action to take if any
enum cm_check_rv {
// Return value indicating that everything is in order and no action is required
    CHECK_RV_CLEAN = 0,
// Return value indicating that the check failed
    CHECK_RV_FAILURE = 1,
// Return value indicating that it is necessary to send a Log Repair request to P
    CHECK_RV_NEED_REPAIR = 2,
// Return value indicating that it is necessary to send a Log Replay request to P
    CHECK_RV_NEED_REPLAY = 3,
// Return value indicating that it is necessary to reply an Indicate P CM to the sender
    CHECK_RV_NEED_INDICATE_P = 4,
// Return value indicating that it is necessary to reply with a transmission of the latest entry to the sender
    CHECK_RV_NEED_TR_LATEST = 5,
// Return value indicating that it is necessary to reply with a transmission of the entry corresponding to
// the sender's next index
    CHECK_RV_NEED_TR_NEXT = 6,
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
    // Sender's commit index
    uint64_t commit_index;
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
