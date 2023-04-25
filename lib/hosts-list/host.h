//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_HOST_H
#define THESIS_CSEC_HOST_H

#include <netinet/in.h>

#define HOST_STATUS_UNKNOWN 0 // Unknown, which is the default value after the hosts list is initialized
#define HOST_STATUS_P 1 // P master
#define HOST_STATUS_HS 2 // HS master
#define HOST_STATUS_CS 3 // CS master
#define HOST_STATUS_S 4 // Server
#define HOST_STATUS_UNREACHABLE (-1) // Unreachable node
#define HOST_STATUS_UNRESOLVED (-2) // Its address couldn't be resolved at start

typedef struct host_s {
    uint8_t status;
    struct sockaddr_storage addr_storage;
    char addr_string[256]; // String parsed by the hosts_list initializer corresponding to this host entry
} host_s;

#endif //THESIS_CSEC_HOST_H
