//
// Created by zheraan on 19/06/23.
//

#include "elections.h"

void election_state_reset(election_state_s *es) {
    es->candidacy = NOT_CANDIDATE;
    es->vote_count = 0;
    es->last_voted_bid = 0;
    es->bid_number = 0;
    return;
}

int start_hs_candidacy(overseer_s *overseer) {

    return EXIT_SUCCESS;
}

int stop_hs_candidacy(overseer_s *overseer) {
    overseer->es->vote_count = 0; // Reset local vote count
    if (overseer->es->candidacy == NOT_CANDIDATE)
        return EXIT_SUCCESS;
    overseer->es->candidacy = NOT_CANDIDATE; // Reset candidacy

    return EXIT_SUCCESS;
}

int stepdown_to_cs(overseer_s *overseer) {

    return EXIT_SUCCESS;
}

int promote_to_hs(overseer_s *overseer) {
    election_state_reset(overseer->es);

    return EXIT_SUCCESS;
}

int promote_to_p(overseer_s *overseer) {

    return EXIT_SUCCESS;
}