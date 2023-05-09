//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_HOSTS_LIST_H
#define THESIS_CSEC_HOSTS_LIST_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <event2/util.h>
#include "host.h"

#ifndef HOSTS_LIST_SIZE
#define HOSTS_LIST_SIZE 128 // May be redefined
#endif

#ifdef NO_DNS_LOOKUP // May be defined to skip DNS lookup
#define NO_DNS_LOOKUP_ 1
#else
#define NO_DNS_LOOKUP_ 0
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 2
#endif

#ifndef ENO_P
#define ENO_P (-89)
#endif

typedef struct hosts_list_s {
    host_s hosts[HOSTS_LIST_SIZE]; //TODO Replace by a hashtable
    uint32_t localhost_id;
    uint32_t nb_hosts;
} hosts_list_s;

// Returns the number of hosts parsed from the hostfile (one address per row) into the list.
uint32_t hosts_init(char const *hostfile, hosts_list_s *list);

// Attempts to re-resolve a host that was not successfully resolved at startup
// Returns EXIT_SUCCESS on success, and EXIT_FAILURE on failure
int host_re_resolve(hosts_list_s *list, uint32_t host_id);

// Returns nonzero iff row is a string containing only whitespace (or is empty)
int is_blank(char const *line);

// Returns nonzero iff row starts with a '#' and therefore is a comment
int is_comment(char const *line);

// Returns 1 if there is a node with HOST_STATUS_P in the list, 0 otherwise
int is_p_available(hosts_list_s *list);

// Returns the ID of the first node with P status encountered, otherwise returns 1 and sets errno to ENO_P
uint32_t whois_p(hosts_list_s *list);

#endif //THESIS_CSEC_HOSTS_LIST_H
