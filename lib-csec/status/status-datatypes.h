//
// Created by zheraan on 18/07/23.
//

#ifndef THESIS_CSEC_STATUS_DATATYPES_H
#define THESIS_CSEC_STATUS_DATATYPES_H

#include "stdlib.h"
#include "event2/event.h"

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

#endif //THESIS_CSEC_STATUS_DATATYPES_H
