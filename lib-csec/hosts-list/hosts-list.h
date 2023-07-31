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
#include "hl-datatypes.h"
#include "../overseer.h"
#include "../status/elections.h"

#ifndef HOSTS_LIST_SIZE
#define HOSTS_LIST_SIZE 128 // May be redefined
#endif

#ifdef NO_DNS_LOOKUP // May be defined to skip DNS lookup
#define NO_DNS_LOOKUP_ 1
#else
#define NO_DNS_LOOKUP_ 0
#endif

#ifndef ENONE
#define ENONE (-89)
#endif

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

// Returns the ID of the first node with given status encountered, otherwise returns EXIT_FAILURE and
// sets errno to ENONE.
uint32_t hl_whois(hosts_list_s *list, enum host_status status);

// Resets any master node with the given status to CS, and sets the node with the given ID to the given status
// If the concerned node was the local host, calls stepdown_to_cs.
// Returns EXIT_SUCCESS, or EXIT_FAILURE if id or status parameters are invalid.
int hl_change_master(overseer_s *overseer, enum host_status status, uint32_t id);

// Changes the status of the target node in the list to the one given in parameter, calls hl_change_master if
// it concerns HS or P.
// Returns EXIT_SUCCESS, or EXIT_FAILURE if id parameter is invalid.
int hl_update_status(overseer_s *overseer, enum host_status status, uint32_t id);

// Changes the local known indexes of target host, and adjust the replication indexes for each concerned log
// entry.
int hl_replication_index_change(overseer_s *overseer, uint32_t host_id, uint64_t next_index, uint64_t commit_index);

#endif //THESIS_CSEC_HOSTS_LIST_H
