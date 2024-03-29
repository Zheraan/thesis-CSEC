//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_ENTRY_TRANSMISSION_H
#define THESIS_CSEC_ENTRY_TRANSMISSION_H

#include <stdlib.h>
#include <stdint.h>
#include "rc-datatypes.h"
#include "../raft-log/log.h"
#include "control-message.h"
#include "../expe/fuzzer.h"

#ifndef PROPOSITION_RETRANSMISSION_DEFAULT_ATTEMPTS
// Defines how many times should the server attempt to retransmit a proposition before ditching it
#define PROPOSITION_RETRANSMISSION_DEFAULT_ATTEMPTS 2 \
// TODO Extension Label P is unreachable if too many attempts failed, or transition to partition mode
#endif

// Allocates a new transmission struct and initializes it with the parameters. The contents of the data op
// are copied in the transmission, so they can (and have to) be freed at some point afterward.
// Returns NULL if the allocation fails, or a pointer to the newly allocated struct.
entry_transmission_s *etr_new(const overseer_s *overseer,
                              enum message_type type,
                              const data_op_s *op,
                              uint64_t index,
                              uint32_t term,
                              enum entry_state state,
                              uint32_t ack_back);

// Allocates a new transmission struct and initializes it with the parameters. The contents of the data op
// are copied in the transmission from the local log entry corresponding to the given id, if any.
// Returns NULL if the allocation fails or if no entry was found with this id, or a pointer to the
// newly allocated struct.
entry_transmission_s *etr_new_from_local_entry(const overseer_s *overseer,
                                               enum message_type type,
                                               uint64_t entry_id,
                                               uint32_t ack_back);

// Outputs the contents of the structure to the specified stream
// The flags parameter may be CSEC_FLAG_DEFAULT (zero) or FLAG_PRINT_SHORT for a shorter inline print
void etr_print(const entry_transmission_s *etr, FILE *stream, int flags);

// Sends a transmission to the specified address
// The flags parameter at the moment is only used to eventually bypass the fuzzer if an ETR is destined at
// a CM node. Values can be CSEC_FLAG_DEFAULT (zero) or FLAG_BYPASS_FUZZER
// Returns either EXIT_SUCCESS or EXIT_FAILURE
int
etr_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, entry_transmission_s *etr, int flags);

// Sends an entry transmission to the specified address, caches it for retransmission, and sets the callback
// event for it. The message sent will have the same ack_reference as the retransmission cache id for that ETR.
// The associated etr will be automatically freed along with its cache entry once it's been resent rt_attempts
// times, and must not be freed manually until then unless the whole cache entry and its related events are
// removed through rtc_remove_by_id.
// The flags parameter at the moment is only used to eventually bypass the fuzzer if an ETR is destined at
// a CM node. Values can be CSEC_FLAG_DEFAULT (zero) or FLAG_BYPASS_FUZZER
// Returns either EXIT_SUCCESS or EXIT_FAILURE
int etr_sendto_with_rt_init(overseer_s *overseer,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            entry_transmission_s *etr,
                            uint8_t rt_attempts,
                            int flags);

// Initializes the entry transmission reception event.
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int etr_reception_init(overseer_s *overseer);

// Callback for receiving transmissions. Processes the incoming transmission and resets the entry
// reception event. Arg must be a pointer to the local overseer.
// TODO If a proposition is sent with this and reaches P, but the Ack doesn't reach back, we have to make
//  sure that duplicate propositions are ignored. It should be checked with next index that is then outdated,
//  however in this case we should not re-send the proposition as it will be done anyway through normal
//  retransmission, as the Ack order for the prop is a new pending entry order from P.
//  FIX: ignore proposition if its index is already occupied by an entry
// FIXME Index of entry will be generated when sent, need to something to guard against duplicates, perhaps
//  retransmit for as long as the entry is live in sender's queue ?
void etr_receive_cb(evutil_socket_t fd, short event, void *arg);

// Callback for the retransmission of ETRs. Arg must be a retransmission_s*. Cleans up the retransmission
// cache once maximum attempts have been reached
// Arg must be a pointer to the corresponding retransmission cache entry
void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg);

// Transmits the entry corresponding to the cm's next index to its sender
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_reply_logfix(overseer_s *overseer, const control_message_s *cm);

// Sends the commit order for the given entry if it's committed, or fails otherwise
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_broadcast_commit_order(overseer_s *overseer, uint64_t index);

// Broadcasts a received new entry in response to its reception, sending a ETR NEW AND ACK to the node that
// first sent it.
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_broadcast_new_entry(overseer_s *overseer, uint64_t index, uint32_t sender_id, uint32_t ack_back);

// Determines the correct actions to take depending on local status and incoming ETR, for all host types.
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_actions(overseer_s *overseer,
                struct sockaddr_in6 sender_addr,
                socklen_t socklen,
                entry_transmission_s *etr);

// Determines the correct actions to take depending on local status and incoming ETR. Used for incoming ETRs
// and a local host of status P.
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_actions_as_p(overseer_s *overseer,
                     struct sockaddr_in6 sender_addr,
                     socklen_t socklen,
                     entry_transmission_s *etr);


// Determines the correct actions to take depending on local status and incoming ETR. Used for incoming ETRs
// and a local host of status S, HS or CS.
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_actions_as_s_hs_cs(overseer_s *overseer,
                           struct sockaddr_in6 sender_addr,
                           socklen_t socklen,
                           entry_transmission_s *etr);

// Determines the correct actions to take depending on local status and incoming ETR. Used for incoming ETRs
// and a local host of status CM.
// Returns EXIT_SUCCESS or EXIT_FAILURE
int etr_actions_as_cm(overseer_s *overseer,
                      struct sockaddr_in6 sender_addr,
                      socklen_t socklen,
                      entry_transmission_s *etr);

// Sends the first element in the proposition queue (added the earliest) and sets its retransmission through
// the retransmission cache. If the queue is empty, has no effect.
// Returns EXIT_FAILURE and wipes the proposition queue in case of failure, returns EXIT_SUCCESS otherwise
// FIXME If a proposition isn't accepted after all retransmission attempts, the queue needs to be wiped to
//  prevent incoherences because of subsequent entries that may depend on the first and succeed
int server_send_first_prop(overseer_s *overseer, uint32_t ack_back);

#endif //THESIS_CSEC_ENTRY_TRANSMISSION_H
