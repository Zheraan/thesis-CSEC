//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_RC_DATATYPES_H
#define THESIS_CSEC_RC_DATATYPES_H

#include <stdint.h>
#include "../raft-log/rl-datatypes.h"
#include "../hosts-list/hl-datatypes.h"
#include "../mocked-fs/mfs-datatypes.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// Slight optimisation on the modulo function in case X has a chance to be smaller than Y
#define MODULO(X, Y)  ((X) < (Y) ? (X) : (X) % (Y))
// Slight optimisation on the divide function in case X has a chance to be smaller than Y
#define DIVIDE(X, Y)  ((X) < (Y) ? (0) : (X) / (Y))

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

#ifndef TIMEOUT_GLOBAL_SLOWDOWN_OFFSET
// Offset for all timeouts to slow down the program without having to fine tune it, for debug purposes.
// Unit in microseconds
#define TIMEOUT_GLOBAL_SLOWDOWN_OFFSET 1000000
#endif

#ifndef TIMEOUT_RANGE_ELECTION
// Timeout duration range for elections. Default value of 500ms, in microseconds
#define TIMEOUT_RANGE_ELECTION 500000
#endif

#ifndef TIMEOUT_OFFSET_ELECTION
// Timeout duration offset for elections. Default value of 1s, in microseconds
#define TIMEOUT_OFFSET_ELECTION 1000000
#endif

#ifndef TIMEOUT_RANGE_RANDOM_OPS
// Timeout duration range for the random op generator. Default value of 12s, in microseconds
#define TIMEOUT_RANGE_RANDOM_OPS 12000000
#endif
#ifndef TIMEOUT_OFFSET_RANDOM_OPS
// Timeout duration offset for the random op generator. Default value of 0ms, in microseconds
#define TIMEOUT_OFFSET_RANDOM_OPS 0
#endif

#ifndef TIMEOUT_VALUE_PROPOSITION
// Timeout duration for propositions. Default value of 3s, in microseconds
#define TIMEOUT_VALUE_PROPOSITION 3000000
#endif

#ifndef TIMEOUT_VALUE_ACK
// Timeout duration for Acks. Default value of 200ms, in microseconds
#define TIMEOUT_VALUE_ACK 200000
#endif

#ifndef TIMEOUT_VALUE_PROP_RETRANSMISSION
// Timeout duration for the proposition retransmission. Default value of 300ms, in microseconds
#define TIMEOUT_VALUE_PROP_RETRANSMISSION 300000
#endif

#ifndef TIMEOUT_VALUE_P_HB
// Timeout duration for P's heartbeat. Default value of 750ms, in microseconds
#define TIMEOUT_VALUE_P_HB 750000
#endif

#ifndef TIMEOUT_VALUE_HS_HB
// Timeout duration for HS's heartbeat. Default value of 750ms, in microseconds
#define TIMEOUT_VALUE_HS_HB 750000
#endif

#ifndef TIMEOUT_VALUE_P_LIVENESS
// Timeout duration for P's liveness. Default value of 500ms, in microseconds
#define TIMEOUT_VALUE_P_LIVENESS 500000
#endif

#ifndef TIMEOUT_RANGE_FUZZER
// Timeout duration range for the fuzzer. Default value of 0.5s, in microseconds
#define TIMEOUT_RANGE_FUZZER 500000
#endif
#ifndef TIMEOUT_OFFSET_FUZZER
// Timeout duration offset for the fuzzer. Default value of 10ms, in microseconds
#define TIMEOUT_OFFSET_FUZZER 10000
#endif

#ifndef FUZZER_LATENCY_DISTRIBUTION_ENABLE
// Enables the use of a more realistic distribution of latencies for the fuzzer
#define FUZZER_LATENCY_DISTRIBUTION_ENABLE 1
#endif

#ifndef FUZZER_LATENCY_DISTRIBUTION_PROPORTION
// Enables the use of a more realistic distribution of latencies for the fuzzer. Must be strictly
// comprised between 0 and 100. Is the percentage of packets with latency below 2*a.
#define FUZZER_LATENCY_DISTRIBUTION_PROPORTION 90
#endif

#ifndef FUZZER_LATENCY_DISTRIBUTION_MAXIMUM
// Maximum latency for the fuzzer latency distribution function, parameter in microseconds. Set 0 for
// no maximum. Default is 255 seconds, commonly assumed to be the maximum segment lifetime for UDP
#define FUZZER_LATENCY_DISTRIBUTION_MAXIMUM 255000000
#endif

#ifndef FUZZER_LATENCY_DISTRIBUTION_MINIMUM
// Minimum latency for the fuzzer latency distribution function, parameter in microseconds
#define FUZZER_LATENCY_DISTRIBUTION_MINIMUM TIMEOUT_OFFSET_FUZZER
#endif

// TODO Improvement enforce 0<A<C<B for the distribution function

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
    // Master election timeout type, after which a round of HS elections starts
    TIMEOUT_TYPE_HS_ELECTION = 4,
    // Fuzzer delay introduction timeout type, after which a message delayed by the fuzzer is sent
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

#include "../overseer.h"

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
    //  to the wrong target anyway. Could cause unexpected and rare issues, need to check if the role is
    //  still the same.

    // Address of the target
    struct sockaddr_in6 addr;
    // Socklen of the target
    socklen_t socklen;
    // Type of message
    enum message_type type;
    // ETR's or CM's ack_back value
    uint32_t ack_back;
    // Pointer to the program state
    overseer_s *overseer;
    // Related event
    struct event *ev;
} retransmission_cache_s;

#endif //THESIS_CSEC_RC_DATATYPES_H
