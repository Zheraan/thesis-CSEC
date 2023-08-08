//
// Created by zheraan on 03/05/23.
//

#ifndef THESIS_CSEC_RETRANSMISSION_CACHE_H
#define THESIS_CSEC_RETRANSMISSION_CACHE_H

#include <event2/event.h>
#include <stdlib.h>

#include "../datatypes.h"
#include "../overseer.h"

#ifndef FLAG_SILENT
#define FLAG_SILENT 0x1
#endif

// Adds a new cache element to the list in the overseer to keep track of messages waiting for an ack that
// also need to be freed later. Given etr parameter MUST be NULL for CM retransmission.
// Returns the ID of the allocated cache RT cache, or 0 otherwise
uint32_t rtc_add_new(overseer_s *overseer,
                     uint8_t rt_attempts,
                     struct sockaddr_in6 sockaddr,
                     socklen_t socklen,
                     enum message_type type,
                     entry_transmission_s *etr,
                     uint32_t ack_back);

// Returns a pointer to the cache element designated by the given ID, or NULL if none is found to match.
retransmission_cache_s *rtc_find_by_id(overseer_s *overseer, uint32_t id);

// Removes the retransmission of all entries of given type in the retransmission cache.
// Returns the number of freed elements.
uint32_t rtc_remove_by_type(overseer_s *overseer, enum message_type type);

// Removes and frees the element in the cache list with the given ID
// Returns EXIT_SUCCESS, or EXIT_FAILURE if cache was empty or did not contain any entry with the given ID.
// If the flag parameter is FLAG_SILENT, does not log an error for empty cache or if no entry with the
// given ID is found. If the id was the same as the local fix_conversation value, sets it back to 0 and
// the fix_type to FIX_TYPE_NONE.
// TODO Improvement: store and check sender in cache to avoid disruption
int rtc_remove_by_id(overseer_s *overseer, uint32_t id, char flag);

// Frees the full cache and deletes associated events
void rtc_free_all(overseer_s *overseer);

// Frees a single cache element and
void rtc_free(retransmission_cache_s *rtc);

#endif //THESIS_CSEC_RETRANSMISSION_CACHE_H
