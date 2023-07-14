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

#ifndef CM_DEFAULT_RT_ATTEMPTS
// Default number of retransmission attempts for Control Messages (in addition to the original transmission)
#define CM_DEFAULT_RT_ATTEMPTS 2
#endif

#ifndef ETR_DEFAULT_RT_ATTEMPTS
// Default number of retransmission attempts for ETRs (in addition to the original transmission)
#define ETR_DEFAULT_RT_ATTEMPTS 2
#endif

// Offset for all timeouts to slow down the program without having to fine tune it, for debug purposes
#ifndef TIMEOUT_GLOBAL_SLOWDOWN_OFFSET_SEC
#define TIMEOUT_GLOBAL_SLOWDOWN_OFFSET_SEC 1
#endif
#ifndef TIMEOUT_GLOBAL_SLOWDOWN_OFFSET_USEC
#define TIMEOUT_GLOBAL_SLOWDOWN_OFFSET_USEC 0
#endif

// Timeout duration range for elections. Default value of 500ms
#ifndef TIMEOUT_RANGE_ELECTION_SEC
#define TIMEOUT_RANGE_ELECTION_SEC 0
#endif
#ifndef TIMEOUT_RANGE_ELECTION_USEC
#define TIMEOUT_RANGE_ELECTION_USEC 500000
#endif
// Timeout duration offset for elections. Default value of 150ms
#ifndef TIMEOUT_OFFSET_ELECTION_SEC
#define TIMEOUT_OFFSET_ELECTION_SEC 0
#endif
#ifndef TIMEOUT_OFFSET_ELECTION_USEC
#define TIMEOUT_OFFSET_ELECTION_USEC 150000
#endif

// Timeout duration range for the fuzzer. Default value of 2s
#ifndef TIMEOUT_RANGE_FUZZER_SEC
#define TIMEOUT_RANGE_FUZZER_SEC 1
#endif
#ifndef TIMEOUT_RANGE_FUZZER_USEC
#define TIMEOUT_RANGE_FUZZER_USEC 999999
#endif
// Timeout duration offset for the fuzzer. Default value of 10ms
#ifndef TIMEOUT_OFFSET_FUZZER_SEC
#define TIMEOUT_OFFSET_FUZZER_SEC 0
#endif
#ifndef TIMEOUT_OFFSET_FUZZER_USEC
#define TIMEOUT_OFFSET_FUZZER_USEC 10000
#endif

// Timeout duration range for the random op generator. Default value of 8s
#ifndef TIMEOUT_RANGE_RANDOM_OPS_SEC
#define TIMEOUT_RANGE_RANDOM_OPS_SEC 7
#endif
#ifndef TIMEOUT_RANGE_RANDOM_OPS_USEC
#define TIMEOUT_RANGE_RANDOM_OPS_USEC 999999
#endif
// Timeout duration offset for the random op generator. Default value of 0ms
#ifndef TIMEOUT_OFFSET_RANDOM_OPS_SEC
#define TIMEOUT_OFFSET_RANDOM_OPS_SEC 0
#endif
#ifndef TIMEOUT_OFFSET_RANDOM_OPS_USEC
#define TIMEOUT_OFFSET_RANDOM_OPS_USEC 0
#endif

// Timeout duration for propositions. Default value of 500ms
#ifndef TIMEOUT_VALUE_PROPOSITION_SEC
#define TIMEOUT_VALUE_PROPOSITION_SEC 0
#endif
#ifndef TIMEOUT_VALUE_PROPOSITION_USEC
#define TIMEOUT_VALUE_PROPOSITION_USEC 500000
#endif

// Timeout duration for Acks. Default value of 170ms
#ifndef TIMEOUT_VALUE_ACK_SEC
#define TIMEOUT_VALUE_ACK_SEC 0
#endif
#ifndef TIMEOUT_VALUE_ACK_USEC
#define TIMEOUT_VALUE_ACK_USEC 170000
#endif

// Timeout duration for the proposition retransmission. Default value of 300ms
#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC 0
#endif
#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC
#define TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC 300000
#endif

// Timeout duration for P's heartbeat. Default value of 1.5s
#ifndef TIMEOUT_VALUE_P_HB_SEC
#define TIMEOUT_VALUE_P_HB_SEC 1
#endif
#ifndef TIMEOUT_VALUE_P_HB_USEC
#define TIMEOUT_VALUE_P_HB_USEC 500000
#endif

// Timeout duration for HS's heartbeat. Default value of 1.5s
#ifndef TIMEOUT_VALUE_HS_HB_SEC
#define TIMEOUT_VALUE_HS_HB_SEC 1
#endif
#ifndef TIMEOUT_VALUE_HS_HB_USEC
#define TIMEOUT_VALUE_HS_HB_USEC 500000
#endif

// Timeout duration for P's liveness. Default value of 3s
#ifndef TIMEOUT_VALUE_P_LIVENESS_SEC
#define TIMEOUT_VALUE_P_LIVENESS_SEC 3
#endif
#ifndef TIMEOUT_VALUE_P_LIVENESS_USEC
#define TIMEOUT_VALUE_P_LIVENESS_USEC 0
#endif


// Control message type values, including those only used within the transmission struct.
// Message types concerning CMs have values <=100, and those concerning ETRs have values >100
enum message_type {
    // CM Message types ---------------------------------------------------------------------------

    // HEARTBEATS
    // Default heartbeat
    MSG_TYPE_HB_DEFAULT = 0,
    // New P taking over
    MSG_TYPE_P_TAKEOVER = 2,
    // New HS taking over
    MSG_TYPE_HS_TAKEOVER = 3,
    // Message broadcasted periodically in case the local node has detected its isolation and is in partition
    // mode, likely because of a network failure.
    // TODO Extension implement partition mode and network probe
    //  - need a way to detect that the network is partitioned even if the partition contains several nodes
    //  - partitions contain up to half the nodes (but not a strict majority)
    MSG_TYPE_NETWORK_PROBE = 4,

    // CONTROL
    // Repairing the log, asking master for entry in next index
    MSG_TYPE_LOG_REPAIR = 10,
    // Replaying the log, asking master for entry in next index
    MSG_TYPE_LOG_REPLAY = 11,
    // Ack without issues, sent back for takeover messages, heartbeats and back from commit or entry acks
    MSG_TYPE_GENERIC_ACK = 20,
    // Ack message for an ETR New
    MSG_TYPE_ACK_ENTRY = 21,
    // Ack message for an ETR Commit
    MSG_TYPE_ACK_COMMIT = 22,
    // Message for indicating who is P in case host that is not P received a message sent to P or if the
    // sender of a message had an outdated P-term
    MSG_TYPE_INDICATE_P = 30,
    // Message for indicating who is HS in case host that is not HS received a message sent to HS or if
    // the sender of a message had an outdated HS-term
    MSG_TYPE_INDICATE_HS = 31,

    // STATUS
    // Message for indicating candidacy to be the next HS to other nodes
    MSG_TYPE_HS_VOTING_BID = 40,
    // Message for giving a vote to another node
    MSG_TYPE_HS_VOTE = 41,

    // ETR Message types (only valid within transmissions) ----------------------------------------
    // Master sending a new entry to add to the log as pending
    MSG_TYPE_ETR_NEW = 101,
    // Master sending the commit order for a pending entry (and the entry itself)
    MSG_TYPE_ETR_COMMIT = 102,
    // Server sending a new entry proposition to P
    MSG_TYPE_ETR_PROPOSITION = 103,
    // Master sending an entry that is NOT a new entry to add as pending, but one sent in order to fix
    // the receiver's log as result of a Log Repair or Log Replay
    MSG_TYPE_ETR_LOGFIX = 104,
    // Master replying to the node sending a proposition, serves as an ack of the node's proposition and as
    // a new pending entry order. Is important to avoid duplicates in a simple way without multiplying messages
    MSG_TYPE_ETR_NEW_AND_ACK = 105,
};

enum timeout_type {
    // P master heartbeat timeout type, after which a new heartbeat is sent
    TIMEOUT_TYPE_P_HB = 0,
    // HS master heartbeat timeout type, after which a new heartbeat is sent
    TIMEOUT_TYPE_HS_HB = 1,
    // Data op proposition timeout type, after which the proposition is deleted from queue
    TIMEOUT_TYPE_PROPOSITION = 2,
    // Message ack timeout type, after which a message is considered to be lost
    TIMEOUT_TYPE_ACK = 3,
    // Master election timeout type
    TIMEOUT_TYPE_ELECTION = 4,
    // Fuzzer delay introduction timeout type
    TIMEOUT_TYPE_FUZZER = 5,
    // Random data op generator timeout type, after which a new random data op is generated and proposed
    TIMEOUT_TYPE_RANDOM_OPS = 6,
    // Timeout before retransmitting cached ops proposition
    TIMEOUT_TYPE_PROP_RETRANSMISSION = 7,
    // Timeout before P is considered unreachable
    TIMEOUT_TYPE_P_LIVENESS = 8,
};

typedef struct control_message_s {
    // Message sender's id in the hosts list
    uint32_t host_id;
    // Status of the sender
    enum host_status status;
    // TODO Improvement Implement target status to avoid complex issues
    // Enum for determining the type of control message
    enum message_type type;
    // Message number to determine what number an ack back for this message should have, in which case it
    // must be the same as the local retransmission cache number. Otherwise, it has to be 0 if the message
    // does not require an acknowledgement.
    uint32_t ack_reference;
    // Message number to acknowledge a precedent message sent by the receptor of this message, in which case
    // it must be the same number as the receptor's message's retransmission cache number. Otherwise, it has
    // to be 0 if the message is not carrying the acknowledgement of a previous message.
    uint32_t ack_back;

    // Sender's next index
    uint64_t next_index;
    // Sender's commit index. In case of a voting bid or vote CM, is replaced with the bid number and
    // therefore should not be used for commit-related checks.
    uint64_t commit_index;
    // Sender's current P_term
    uint32_t P_term;
    // Sender's current HS_term
    uint32_t HS_term;
} control_message_s;

typedef struct entry_transmission_s {
    // Status of the sender and type of transmission
    control_message_s cm;
    // Transmitted entry's related data op
    data_op_s op;
    // Transmitted entry's index
    uint32_t index;
    // Transmitted entry's P-term
    uint32_t term;
    // Term of the entry before transmitted entry
    uint32_t prev_term;
    // Transmitted entry's state
    enum entry_state state;
} entry_transmission_s;

// Struct to hold a message or transmission that needs to be retransmitted and freed later, in a linked list
// fashion
typedef struct retransmission_cache_s {
    // ID of the retransmission. Every new retransmission cache has an ID of the previous element +1;
    // Whenever it hits the maximum value, the next one will be 0 by voluntary overflow.
    uint32_t id;
    // Pointer to the next list element
    struct retransmission_cache_s *next;
    // Transmission that needs to be resent.
    // Control messages can be generated on the spot from the program state and the message type, so it is
    // not necessary to have a pointer for them. However, transmissions are not
    entry_transmission_s *etr;
    // Number of retransmission attempts made
    uint8_t cur_attempts;
    // Max number of retransmissions
    uint8_t max_attempts;
    // FIXME If a message destined to HS/P needs to be resent but target role's change, the message is resent
    //  to the wrong target anyway. Can cause unexpected issues, need to check if the role is still the same.
    // Address of the target
    struct sockaddr_in6 addr;
    // Socklen of the target
    socklen_t socklen;
    // Type of message
    enum message_type type;
    // Type of message
    uint32_t ack_back;
    // Pointer to the program state
    overseer_s *overseer;
    // Related event
    struct event *ev;
} retransmission_cache_s;

#endif //THESIS_CSEC_RC_DATATYPES_H
