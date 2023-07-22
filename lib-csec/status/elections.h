//
// Created by zheraan on 19/06/23.
//

#ifndef THESIS_CSEC_ELECTIONS_H
#define THESIS_CSEC_ELECTIONS_H

#include "status-datatypes.h"
#include "../overseer.h"
#include "master-events.h"

#ifndef PARTITION_THRESHOLD
#define PARTITION_THRESHOLD 3
#endif

// Resets the election state values to default values and a new timeout for CS nodes, or removes it for other
// types.
void election_state_reset(overseer_s *overseer);

// Starts a round of HS candidacy by increasing bid number, resets votes, broadcasting a bid to master nodes, then
// resetting the timeout for the next round.
// Note: vote count is set at one since the local host votes for itself.
int start_hs_candidacy_bid(overseer_s *overseer);

// Difference with election_state_reset is that the bid number and last voted round are left unchanged here.
// Calls election_set_timeout to set the next round.
void end_hs_candidacy_round(overseer_s *overseer);

// Updates the local node status to be CS, removes the master heartbeat if set, removes P liveness if set,
// calls election_state_reset (which adds election timer).
// Has no effect if local host is the only master node.
int stepdown_to_cs(overseer_s *overseer);

// Updates the local node status to be HS, removes the election event, sets the master heartbeat, and
// broadcasts a takeover message to all master nodes.
// Calls promote_to_p if local host is the only master node
int promote_to_hs(overseer_s *overseer);


// Updates the local node status to be P, removes the election event, sets the master heartbeat, removes
// P liveness monitoring and broadcasts a takeover message to all master nodes.
// Calls promote_to_p if local host is the only master node
int promote_to_p(overseer_s *overseer);

// (re)sets the timeout event for the elections.
// Always returns EXIT_SUCCESS
int election_set_timeout(overseer_s *overseer);

// Callback for receiving control messages, arg must be an overseer_s*. Has no effect if local is not CS.
void election_timeout_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_ELECTIONS_H
