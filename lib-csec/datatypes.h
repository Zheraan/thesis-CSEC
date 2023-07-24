//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_DATATYPES_H
#define THESIS_CSEC_DATATYPES_H

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4 // Defines the level of details in the logs
#endif

#ifndef INSTANT_FFLUSH
#define INSTANT_FFLUSH 1
#endif

#ifndef FUZZER_ENABLED
#define FUZZER_ENABLED 1 // Boolean value determining if the fuzzer is used or not
#endif

#ifndef FUZZER_MESSAGE_DROPPING
// Boolean value determining if the fuzzer should drop messages using the set drop rate FUZZER_DROP_RATE
#define FUZZER_MESSAGE_DROPPING 1
#endif

#ifndef FUZZER_DROP_RATE
#define FUZZER_DROP_RATE 20 // Chances of the fuzzer dropping messages, in percentage
#endif

enum entry_state;
enum timeout_type;
enum message_type;
enum host_locality;
enum node_type;
enum host_status;
enum fix_type;
enum candidacy_type;
enum packet_type;

union packet;

typedef struct overseer_s overseer_s;
typedef struct host_s host_s;
typedef struct hosts_list_s hosts_list_s;
typedef struct data_op_s data_op_s;
typedef struct ops_queue_s ops_queue_s;
typedef struct mocked_fs_s mocked_fs_s;
typedef struct control_message_s control_message_s;
typedef struct entry_transmission_s entry_transmission_s;
typedef struct log_entry_s log_entry_s;
typedef struct log_s log_s;
typedef struct retransmission_cache_s retransmission_cache_s;
typedef struct election_state_s election_state_s;
typedef struct fuzzer_cache_s fuzzer_cache_s;

#include "raft-comms/rc-datatypes.h"
#include "raft-log/rl-datatypes.h"
#include "mocked-fs/mfs-datatypes.h"
#include "hosts-list/hl-datatypes.h"
#include "status/status-datatypes.h"

// Program state structure.
// Contains pointers to its composing data structures and the communication socket
struct overseer_s {
    // Program state structures
    struct event_base *eb;
    hosts_list_s *hl;
    log_s *log;
    retransmission_cache_s *rtc;
    mocked_fs_s *mfs;
    election_state_s *es;
    fuzzer_cache_s *fc;

    // Pointer to keep track of heartbeat events for master nodes, and random op generator for server nodes
    struct event *special_event;
    // Pointer to keep track of control message reception events
    struct event *cm_reception_event;
    // Pointer to keep track of entry transmission reception events
    struct event *etr_reception_event;
    // Pointer to keep track of P liveness timeout events
    struct event *p_liveness_event;
    // Socket for sending and receiving control messages
    evutil_socket_t socket_cm;
    // Socket for sending and receiving transmissions
    evutil_socket_t socket_etr;
};

#endif //THESIS_CSEC_DATATYPES_H
