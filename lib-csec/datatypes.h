//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_DATATYPES_H
#define THESIS_CSEC_DATATYPES_H

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

enum entry_state;
enum timeout_type;
enum message_type;
enum host_locality;
enum node_type;
enum host_status;

typedef struct overseer_s overseer_s;
typedef struct host_s host_s;
typedef struct hosts_list_s hosts_list_s;
typedef struct data_op_s data_op_s;
typedef struct ops_queue_s ops_queue_s;
typedef struct mocked_fs_s mocked_fs_s;
typedef struct control_message_s control_message_s;
typedef struct transmission_s transmission_s;
typedef struct log_entry_s log_entry_s;
typedef struct log_s log_s;
typedef struct retransmission_cache_s retransmission_cache_s;

#include "mocked-fs/mfs-datatypes.h"
#include "hosts-list/hl-datatypes.h"
#include "raft-log/rl-datatypes.h"
#include "raft-comms/rc-datatypes.h"

// Program state structure.
// Contains pointers to its composing data structures and the communication socket
struct overseer_s {
    struct event_base *eb;
    hosts_list_s *hl;
    log_s *log;
    retransmission_cache_s *rt_cache;
    mocked_fs_s *mfs;
    // Pointer to keep track of heartbeat events for master nodes, and random op generator for server nodes
    struct event *special_event;
    // Pointer to keep track of control message reception events
    struct event *cm_reception_event;
    // Socket for sending and receiving control messages
    evutil_socket_t socket_cm;
    // Socket for sending and receiving transmissions
    evutil_socket_t socket_tr;
};

#endif //THESIS_CSEC_DATATYPES_H
