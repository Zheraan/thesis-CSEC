//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_HL_DATATYPES_H
#define THESIS_CSEC_HL_DATATYPES_H

#include <netinet/in.h>

#ifndef HOSTS_LIST_SIZE
#define HOSTS_LIST_SIZE 128 // May be redefined
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// Status of a host, if it is available and what type is it if so
enum host_status {
    // Unknown, which is the default value after the hosts list is initialized
    HOST_STATUS_UNKNOWN = 0,
    // P master
    HOST_STATUS_P = 1,
    // HS master
    HOST_STATUS_HS = 2,
    // CS master
    HOST_STATUS_CS = 3,
    // Server
    HOST_STATUS_S = 4,
    // Unreachable node
    HOST_STATUS_UNREACHABLE = -1,
    // Its address couldn't be resolved at start
    HOST_STATUS_UNRESOLVED = -2
};

// Node type of host
enum node_type {
    // For master node
    NODE_TYPE_M = 0,
    // For server node
    NODE_TYPE_S = 1,
    // For cluster monitor node
    NODE_TYPE_CM = 2,
};

// To identify which is the local host
enum host_locality {
    HOST_LOCALITY_LOCAL = 0,
    HOST_LOCALITY_DISTANT = 1,
};

typedef struct host_s {
    // Given name of the host, for legibility and simplification purposes
    char name[256];
    // What is the status of this host ?
    enum host_status status;
    // What type of node is this host ?
    enum node_type type;
    // Is this host the local host ?
    enum host_locality locality;
    // IPv6 address of the host
    struct sockaddr_in6 addr;
    // Address length of the host
    socklen_t socklen;
    // String parsed by the hosts_list initializer corresponding to this host entry, used for re-resolution
    char addr_string[256];
} host_s;

typedef struct hosts_list_s {
    // Array of hosts
    host_s hosts[HOSTS_LIST_SIZE]; //TODO Replace by a hashtable
    // ID of the localhost in the list
    uint32_t localhost_id;
    // Number of hosts in the list
    uint32_t nb_hosts;
} hosts_list_s;

#endif //THESIS_CSEC_HL_DATATYPES_H
