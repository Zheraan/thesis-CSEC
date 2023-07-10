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
    NOT_CANDIDATE = 0,
    HS_CANDIDATE = 1,
};

typedef struct election_state_s {
    uint32_t bid_number;
    uint32_t last_voted_bid;
    uint32_t vote_count;
    enum candidacy_type candidacy;
} election_state_s;

// Resets the election state values to default values
void election_state_reset(election_state_s *es);

int start_hs_candidacy(overseer_s *overseer); // TODO Needed

int stop_hs_candidacy(overseer_s *overseer); // TODO Needed

int stepdown_to_cs(overseer_s *overseer); // TODO Needed

int promote_to_hs(overseer_s *overseer); // TODO Needed

int promote_to_p(overseer_s *overseer); // TODO Needed

#endif //THESIS_CSEC_ELECTIONS_H
