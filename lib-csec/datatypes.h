//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_DATATYPES_H
#define THESIS_CSEC_DATATYPES_H

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4 // Defines the level of details in the logs
#endif

#ifndef PORT_CM
#define PORT_CM 35007
#endif

#ifndef PORT_ETR
#define PORT_ETR 35008
#endif

#ifndef PORT_PSTR
#define PORT_PSTR PORT_CM
#endif

#define true 1
#define false 0

#ifndef INSTANT_FFLUSH
// If this is non-zero, streams will be flushed after each piece of log outputted to stdout or stderr
#define INSTANT_FFLUSH 1
#endif

#ifndef FUZZER_ENABLED
// Boolean value determining if the fuzzer is used or not
#define FUZZER_ENABLED 1
#endif

#ifndef MESSAGE_DROPPING_ENABLED
// Boolean value determining if messages should be dropped using the set drop rate MESSAGE_DROP_RATE
#define MESSAGE_DROPPING_ENABLED 1
#endif

#ifndef MESSAGE_DROP_RATE
#define MESSAGE_DROP_RATE 20 // Chances of the fuzzer dropping messages, in percentage
#endif

#define CSEC_FLAG_DEFAULT 0
#define FLAG_PRINT_SHORT 0x1

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
typedef struct program_state_transmission_s program_state_transmission_s;

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
    uint32_t rtc_index; // Index of the next rtc entry
    uint32_t rtc_number; // Number of rtc entries currently in the cache
    mocked_fs_s *mfs;
    election_state_s *es;
    fuzzer_cache_s *fc;

    // Pointer to keep track of heartbeat events for master nodes, random op generator for server nodes,
    // and PSTR reception for cluster monitor nodes
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
