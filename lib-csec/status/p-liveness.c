//
// Created by zheraan on 14/07/23.
//

#include "p-liveness.h"

int p_liveness_set_timeout(overseer_s *overseer) {
    if (overseer->p_liveness_event != NULL)
        debug_log(4, stdout, "- Re-initializing next P Liveness timeout event ... ");
    else debug_log(4, stdout, "- Initializing next P Liveness timeout event ... ");

    struct event *p_liveness_event = evtimer_new(overseer->eb,
                                                 p_liveness_timeout_cb,
                                                 (void *) overseer);
    if (p_liveness_event == NULL) {
        fprintf(stderr, "Fatal error: failed to create the P Liveness timeout event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // P Liveness has low priority
    event_priority_set(p_liveness_event, 1);

    if (overseer->p_liveness_event != NULL) // Freeing the past event if any
        event_free(overseer->p_liveness_event);
    overseer->p_liveness_event = p_liveness_event;

    // Add the event in the loop
    errno = 0;
    struct timeval p_liveness_timeout = timeout_gen(TIMEOUT_TYPE_P_LIVENESS);
    if (errno == EUNKNOWN_TIMEOUT_TYPE || event_add(p_liveness_event, &p_liveness_timeout) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next Election Timeout event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void p_liveness_timeout_cb(evutil_socket_t fd, short event, void *arg) {

    // Set P's status as Unreachable
    errno = 0;
    uint32_t p_id = hl_whois(((overseer_s *) arg)->hl, HOST_STATUS_P);
    if (errno != ENONE) {
        debug_log(1, stdout, "P liveness timed out.\n");
        hl_update_status((overseer_s *) arg, HOST_STATUS_UNREACHABLE, p_id);
    }

    // If local is HS
    if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status == HOST_STATUS_HS)
        promote_to_p((overseer_s *) arg); // Promote to P
    else p_liveness_set_timeout((overseer_s *) arg); // Otherwise set the next timeout

    return;
}