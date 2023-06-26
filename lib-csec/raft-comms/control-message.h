//
// Created by zheraan on 19/04/23.
//

#ifndef THESIS_CSEC_CONTROL_MESSAGE_H
#define THESIS_CSEC_CONTROL_MESSAGE_H

#include <stdio.h>
#include <event2/util.h>
#include <event2/event.h>
#include "rc-datatypes.h"
#include "../hosts-list/hosts-list.h"
#include "timeout.h"
#include "../overseer.h"
#include "../stepdown.h"

// Allocates a new control message struct and initializes it with the overseer's values. Returns NULL in case
// memory allocation fails.
// If message is of type INDICATE P, host id in the message is replaced by the id of the P node in the local
// host's hosts list. If no node has P status in this case, NULL is returned.
control_message_s *cm_new(const overseer_s *overseer, enum message_type type, uint32_t ack_back);

// Outputs the state of the structure to the specified stream
void cm_print(const control_message_s *hb, FILE *stream);

// Sends a control message to the provided address with the provided message type, without retransmission
// or acknowledgement.
// Returns EXIT_SUCCESS or EXIT_FAILURE depending on result
int cm_sendto(overseer_s *overseer,
              struct sockaddr_in6 sockaddr,
              socklen_t socklen,
              enum message_type type);

// Sale as above, but with an acknowledgement of a previous message.
int cm_sendto_with_ack_back(overseer_s *overseer,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            enum message_type type,
                            uint32_t ack_back);

// Sends a CM, then initializes events and structure in the cache for retransmitting a CM if the number
// of attempts is greater than 0. If the ack reference is non-zero, uses that reference number when sending
// the message and does not create a new retransmission cache. Therefore, only calls concerning a
// message with a pre-existing retransmission cache should use a non-zero value for the ack reference.
// If this control message is sent to acknowledge a previous message from the receiver, the ack_back number
// should be set to that message's ack_reference value, or 0 otherwise. The rt_attempts parameter is
// ignored if the ack_reference parameter is non-zero, as the retransmission cache is handling the
// retransmission attempts.
// Returns EXIT_SUCCESS or EXIT_FAILURE depending on result
int cm_sendto_with_rt_init(overseer_s *overseer,
                           struct sockaddr_in6 sockaddr,
                           socklen_t socklen,
                           enum message_type type,
                           uint8_t rt_attempts,
                           uint32_t ack_reference,
                           uint32_t ack_back);

// Initializes the control message reception event.
// Returns EXIT_FAILURE and prints the reason to stderr in case of failure, EXIT_SUCCESS otherwise
int cm_reception_init(overseer_s *overseer);

// Callback for receiving control messages, arg must be an overseer_s*
void cm_receive_cb(evutil_socket_t fd, short event, void *arg);

// Checks the metadata in the control message and returns a value indicating if any action needs to be taken.
// In case the message's P-term equals the local's, commits any uncommitted data that needs to be.
// Also updates the sender's status in the hosts-list.
enum cm_check_rv cm_check_metadata(overseer_s *overseer, const control_message_s *hb);

// Depending on the return value of cm_check_metadata(), takes the appropriate action
int cm_check_action(overseer_s *overseer,
                    enum cm_check_rv check_rv,
                    struct sockaddr_in6 addr,
                    socklen_t socklen,
                    control_message_s *cm);

// Callback for retransmitting events, cleans up after the maximum number of attempts has been made.
// Arg must be an overseer_s*
void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg);

int cm_actions(overseer_s *overseer,
               struct sockaddr_in6 sender_addr,
               socklen_t socklen,
               control_message_s *cm);

int hb_actions_as_master(overseer_s *overseer,
                         struct sockaddr_in6 sender_addr,
                         socklen_t socklen,
                         control_message_s *cm);

int hb_actions_as_server(overseer_s *overseer,
                         struct sockaddr_in6 sender_addr,
                         socklen_t socklen,
                         control_message_s *cm);

int cm_other_actions_as_s_cs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm);

int cm_other_actions_as_p_hs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm);

// Sends the given CM to target host
// Returns EXIT_SUCCESS or EXIT_FAILURE
int cm_forward(const overseer_s *overseer,
               struct sockaddr_in6 sockaddr,
               socklen_t socklen,
               const control_message_s *cm);

#endif //THESIS_CSEC_CONTROL_MESSAGE_H
