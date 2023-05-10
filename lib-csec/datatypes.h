//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_DATATYPES_H
#define THESIS_CSEC_DATATYPES_H

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
typedef struct event_list_s event_list_s;

#include "mocked-fs/mfs-datatypes.h"
#include "hosts-list/hl-datatypes.h"
#include "raft-log/rl-datatypes.h"
#include "raft-comms/rc-datatypes.h"

// Linked list for keeping track of persistent events that need to be freed at some point
typedef struct event_list_s {
    struct event *e;
    struct event_list_s *next;
} event_list_s;

// Program state structure.
// Contains pointers to its composing data structures and the communication socket
struct overseer_s {
    struct event_base *eb;
    hosts_list_s *hl;
    log_s *log;
    event_list_s *el;
    mocked_fs_s *mfs;
    // Socket for sending and receiving control messages
    evutil_socket_t socket_cm;
    // Socket for sending and receiving transmissions
    evutil_socket_t socket_tr;
};

#endif //THESIS_CSEC_DATATYPES_H
