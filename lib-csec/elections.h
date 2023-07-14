//
// Created by zheraan on 19/06/23.
//

#ifndef THESIS_CSEC_ELECTIONS_H
#define THESIS_CSEC_ELECTIONS_H

#include "overseer.h"

#ifndef PARTITION_THRESHOLD
#define PARTITION_THRESHOLD 3
#endif

enum candidacy_type {
    CANDIDACY_NONE = 0,
    CANDIDACY_HS = 1,
};

typedef struct election_state_s {
    // Current bid the local node is in, for the next HS-term
    uint32_t bid_number;
    // Last bid this note has voted for
    uint32_t last_voted_bid;
    // Number of votes the local host has received for its current bid
    uint32_t vote_count;
    enum candidacy_type candidacy;
    // Pointer to keep track of the election timeouts
    struct event *election_round_event;
} election_state_s;

// Resets the election state values to default values and a new timeout for CS nodes
void election_state_reset(overseer_s *overseer);

// Starts a round of HS candidacy by increasing bid number, resets votes, broadcasting a bid to master nodes, then
// resetting the timeout for the next round.
// Note: vote count is set at one since the local host votes for itself.
int start_hs_candidacy_round(overseer_s *overseer);

// Difference with election_state_reset is that the bid number and last voted round are left unchanged here.
// Calls election_set_timeout to set the next round.
void end_hs_candidacy_round(overseer_s *overseer);

int stepdown_to_cs(overseer_s *overseer); // TODO Needed

int promote_to_hs(overseer_s *overseer); // TODO Needed

int promote_to_p(overseer_s *overseer); // TODO Needed

// (re)sets the timeout event for the elections.
void election_set_timeout(overseer_s *overseer);

// Callback for receiving control messages, arg must be an overseer_s*
void election_timeout_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_ELECTIONS_H
