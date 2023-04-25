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
#include "host.h"

#ifndef HOSTS_LIST_SIZE
#define HOSTS_LIST_SIZE 128 // May be redefined
#endif


typedef struct hosts_list_s {
    host_s hosts[HOSTS_LIST_SIZE];
    uint32_t nb_hosts;
} hosts_list_s;

// Returns the number of hosts parsed from the hostfile (one address per line) into the list.
uint32_t init_hosts(char const *hostfile, hosts_list_s *list);

// Attempts to re-resolve a host that was not successfully resolved at startup
// Returns EXIT_SUCCESS on success, and EXIT_FAILURE on failure
int re_resolve_host(hosts_list_s *list, uint32_t id_host);

// Returns nonzero iff line is a string containing only whitespace (or is empty)
int is_blank(char const *line);

#endif //THESIS_CSEC_HOSTS_LIST_H
