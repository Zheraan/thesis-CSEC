//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_HOST_H
#define THESIS_CSEC_HOST_H

#include <netinet/in.h>


enum host_status{
    HOST_STATUS_UNKNOWN = 0, // Unknown, which is the default value after the hosts list is initialized
    HOST_STATUS_P = 1, // P master
    HOST_STATUS_HS = 2, // HS master
    HOST_STATUS_CS = 3, // CS master
    HOST_STATUS_S = 4, // Server
    HOST_STATUS_UNREACHABLE = -1, // Unreachable node
    HOST_STATUS_UNRESOLVED = -2 // Its address couldn't be resolved at start
};

enum host_type{
    HOST_TYPE_M = 0, // For master node
    HOST_TYPE_S = 1, // For server node
    HOST_TYPE_CM = 2, // For cluster monitor node
};

enum host_locality{
    HOST_LOCALITY_LOCAL = 0,
    HOST_LOCALITY_DISTANT = 1,
};

typedef struct host_s {
    char name[256];
    enum host_status status;
    enum host_type type;
    enum host_locality locality;
    struct sockaddr_in6 addr;
    socklen_t addr_len;
    char addr_string[256]; // String parsed by the hosts_list initializer corresponding to this host entry
} host_s;

#endif //THESIS_CSEC_HOST_H
