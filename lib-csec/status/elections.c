//
// Created by zheraan on 19/06/23.
//

#include "elections.h"

void election_state_reset(overseer_s *overseer) {
    debug_log(4, stdout, "Resetting election state.\n");
    overseer->es->candidacy = CANDIDACY_NONE;
    overseer->es->vote_count = 0;
    overseer->es->last_voted_bid = 0;
    overseer->es->bid_number = 0;
    if (overseer->es->election_round_event != NULL)
        event_free(overseer->es->election_round_event);
    if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_CS)
        election_set_timeout(overseer);
    else overseer->es->election_round_event = NULL;
    return;
}

int start_hs_candidacy_bid(overseer_s *overseer) {
    debug_log(4,
              stdout,
              "Start of HS Candidacy Bid ---------------------------------------------------------------------------------\n");

    overseer->es->candidacy = CANDIDACY_HS;
    overseer->es->bid_number++;
    overseer->es->vote_count = 1;
    int count = 0;

    host_s *target;
    // Broadcast bid to master nodes
    for (uint32_t i = 0; i < overseer->hl->nb_hosts; ++i) {
        target = &overseer->hl->hosts[i];

        // If target is not a master node or if it's the local host
        if (target->type != NODE_TYPE_M || target->locality == HOST_LOCALITY_LOCAL)
            continue; // Skip iteration

        if (DEBUG_LEVEL >= 4) {
            printf("\n- CM target: %s\n", target->name);
            if (INSTANT_FFLUSH) fflush(stdout);
        }

        if (cm_sendto_with_rt_init(overseer,
                                   target->addr,
                                   target->socklen,
                                   MSG_TYPE_HS_VOTING_BID,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   0) == EXIT_SUCCESS)
            count++;
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d CM sent).\n", count);
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    election_set_timeout(overseer);

    debug_log(4,
              stdout,
              "End of HS Candidacy Bid -----------------------------------------------------------------------------------\n");
    return EXIT_SUCCESS;
}

void end_hs_candidacy_round(overseer_s *overseer) {
    debug_log(4, stdout, "Resetting vote count and ");
    overseer->es->vote_count = 0; // Reset local vote count
    if (overseer->es->candidacy == CANDIDACY_NONE)
        return;
    debug_log(2, stdout, "Ending candidacy round.\n");

    overseer->es->candidacy = CANDIDACY_NONE; // Reset candidacy

    if (overseer->es->election_round_event != NULL)
        event_free(overseer->es->election_round_event);
    if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_CS)
        election_set_timeout(overseer);
    else overseer->es->election_round_event = NULL;

    return;
}

int stepdown_to_cs(overseer_s *overseer) {
    enum host_status starting_status = overseer->hl->hosts[overseer->hl->localhost_id].status;
    if (starting_status == HOST_STATUS_CS) {
        if (DEBUG_LEVEL >= 4)
            debug_log(4, stdout,
                      "Setting CS state +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        else debug_log(1, stdout, "Setting CS state ... ");
    } else {
        if (DEBUG_LEVEL >= 4)
            debug_log(4, stdout,
                      "Stepping down to CS ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        else debug_log(1, stdout, "Stepping down to CS ... ");
    }

    if (overseer->hl->nb_masters == 1) {
        debug_log(1, stdout, "Local host is the only master, aborting.\n");
        if (DEBUG_LEVEL >= 4)
            debug_log(4, stdout,
                      "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        return EXIT_SUCCESS;
    }

    hl_update_status(overseer, HOST_STATUS_CS, overseer->hl->localhost_id);

    // Resets the elections and automatically adds the corresponding event
    election_state_reset(overseer);

    // Remove Master Heartbeat
    if (overseer->special_event != NULL) {
        event_free(overseer->special_event);
        overseer->special_event = NULL;
    }

    // Remove P liveness event if set (meaning last role was HS)
    if (overseer->p_liveness_event != NULL) {
        event_free(overseer->p_liveness_event);
        overseer->p_liveness_event = NULL;
    }

    // TODO Improvement Reset replication array to avoid having stale data impact future promotions ?

    if (starting_status == HOST_STATUS_CS) {
        if (DEBUG_LEVEL >= 4)
            debug_log(4, stdout,
                      "End of CS setup ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        else debug_log(1, stdout, "End of CS setup.\n");
    } else {
        if (DEBUG_LEVEL >= 4)
            debug_log(4, stdout,
                      "End of step-down +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        else debug_log(1, stdout, "Done.\n");
    }
    return EXIT_SUCCESS;
}

int promote_to_hs(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 4)
        debug_log(4, stdout,
                  "Promoting local node to HS ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    else debug_log(1, stdout, "Promoting local node to HS ... ");

    if (overseer->hl->nb_masters == 1) {
        debug_log(1, stdout, "Local host is only master, promoting to P instead.\n");
        return promote_to_p(overseer);
    }

    hl_update_status(overseer, HOST_STATUS_HS, overseer->hl->localhost_id);

    // Automatically removes the corresponding event
    election_state_reset(overseer);

    if (master_heartbeat_init(overseer) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failure initializing master heartbeat, stepping back down to CS.\n");
        return stepdown_to_cs(overseer);
    }

    cm_broadcast(overseer, MSG_TYPE_HS_TAKEOVER, CM_DEFAULT_RT_ATTEMPTS, FLAG_SKIP_S);

    if (DEBUG_LEVEL >= 4)
        debug_log(4, stdout,
                  "End of HS promotion +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    else debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int promote_to_p(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 4)
        debug_log(4, stdout,
                  "Promoting local node to P ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    else debug_log(1, stdout, "Promoting local node to P ... ");
    // TODO Improvement query for P's liveness before taking over
    hl_update_status(overseer, HOST_STATUS_P, overseer->hl->localhost_id);

    // Remove P liveness event if set (meaning last role was HS)
    if (overseer->p_liveness_event != NULL) {
        event_free(overseer->p_liveness_event);
        overseer->p_liveness_event = NULL;
    }

    // Automatically removes the corresponding event
    election_state_reset(overseer);

    // Add heartbeat if not set
    if (overseer->special_event == NULL) {
        if (master_heartbeat_init(overseer) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failure initializing master heartbeat, stepping back down to CS.\n");
            return stepdown_to_cs(overseer);
        }
    }

    cm_broadcast(overseer, MSG_TYPE_P_TAKEOVER, CM_DEFAULT_RT_ATTEMPTS, FLAG_NOSKIP);

    if (DEBUG_LEVEL >= 4)
        debug_log(4, stdout,
                  "End of P promotion +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    else debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int election_set_timeout(overseer_s *overseer) {
    if (overseer->es->election_round_event != NULL)
        debug_log(4, stdout, "- Re-initializing next election timeout event ... ");
    else debug_log(4, stdout, "- Initializing next election timeout event ... ");

    struct event *election_event = evtimer_new(overseer->eb,
                                               election_timeout_cb,
                                               (void *) overseer);
    if (election_event == NULL) {
        fprintf(stderr, "Fatal error: failed to create the next election event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Elections have low priority
    event_priority_set(election_event, 1);

    if (overseer->es->election_round_event != NULL) // Freeing the past event if any
        event_free(overseer->es->election_round_event);
    overseer->es->election_round_event = election_event;

    // Add the event in the loop
    errno = 0;
    struct timeval election_timeout = timeout_gen(TIMEOUT_TYPE_HS_ELECTION);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(election_event, &election_timeout) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next Election Timeout event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

// Callback for when an election times out, arg must be an overseer_s*
void election_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status == HOST_STATUS_CS) {
        debug_log(2, stdout, "Election timeout: starting new HS bid.\n");
        start_hs_candidacy_bid((overseer_s *) arg);
    } else {
        debug_log(0, stderr, "Error: an election should not timeout if the local host is not CS.\n");
        exit(EXIT_FAILURE);
    }
    return;
}