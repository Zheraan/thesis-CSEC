//
// Created by zheraan on 03/05/23.
//

#ifndef THESIS_CSEC_RETRANSMISSION_CACHE_H
#define THESIS_CSEC_RETRANSMISSION_CACHE_H

#include <event2/event.h>
#include <stdlib.h>

#include "../datatypes.h"
#include "../overseer.h"

// Adds a new cache element to the list in the overseer to keep track of messages waiting for an ack that
// also need to be freed later. The event needs to be added to it manually
// Returns EXIT_FAILURE in case allocation fails, EXIT_SUCCESS otherwise // TODO rewrite
int rt_cache_add_new(overseer_s *overseer,
                     uint8_t attempts,
                     struct sockaddr_in6 addr,
                     socklen_t socklen,
                     enum message_type type,
                     transmission_s *tr);

// Returns a pointer to the cache element designated by the given ID, or NULL if none is found to match.
retransmission_cache_s *rt_cache_find_by_id(overseer_s *o, uint16_t id);

// Removes and frees the element in the cache list with the given ID
int rt_cache_remove_by_id(overseer_s *o, uint16_t id);

// Frees the full cache and deletes associated events
void rt_cache_free_all(overseer_s *overseer);

// Frees a single cache element and
void rt_cache_free(retransmission_cache_s *rtc);

#endif //THESIS_CSEC_RETRANSMISSION_CACHE_H
