//
// Created by zheraan on 22/04/23.
//

#include "master-events.h"

void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg) {
    debug_log(4, stdout,
              "Start of HB broadcast callback -----------------------------------------------------------------\n");

    enum host_status local_status = (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id]).status;
    uint8_t flags = FLAG_NOSKIP;

    // Skip server nodes if local is HS
    if (local_status == HOST_STATUS_HS)
        flags |= FLAG_SKIP_S;

    // Skip CS nodes if local is P
    if (local_status == HOST_STATUS_P)
        flags |= FLAG_SKIP_CS;

    cm_broadcast((overseer_s *) arg, MSG_TYPE_HB_DEFAULT, CM_DEFAULT_RT_ATTEMPTS, flags);

    // Set the next event
    if (master_heartbeat_init((overseer_s *) arg) != EXIT_SUCCESS) {
        fprintf(stderr, "Fatal Error: next heartbeat event couldn't be set\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE); // TODO Extension Crash handler
    }

    debug_log(4, stdout,
              "End of HB broadcast callback -------------------------------------------------------------------\n\n");
    return;
}

int master_heartbeat_init(overseer_s *overseer) {
    debug_log(3, stdout, "- Initializing next heartbeat event ... ");

    // Create the event related to the socket
    struct event *hb_event = event_new(overseer->eb,
                                       overseer->socket_cm,
                                       EV_TIMEOUT,
                                       master_heartbeat_broadcast_cb,
                                       (void *) overseer);
    if (hb_event == NULL) {
        fprintf(stderr, "Failed to create the heartbeat event\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        return (EXIT_FAILURE);
    }

    // Heartbeat has high priority
    event_priority_set(hb_event, 0);

    if (overseer->special_event != NULL) // Freeing the past heartbeat if any
        event_free(overseer->special_event);
    overseer->special_event = hb_event;

    // Using the right timeout value depending on type
    struct timeval sender_timeout;
    if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P)
        sender_timeout = timeout_gen(TIMEOUT_TYPE_P_HB);
    else if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_HS)
        sender_timeout = timeout_gen(TIMEOUT_TYPE_HS_HB);
    else fprintf(stderr, "bro wtf\n"); // Shouldn't be possible

    // Add the event in the loop
    if (event_add(hb_event, &sender_timeout) != 0) {
        fprintf(stderr, "Failed to add the heartbeat event\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        return (EXIT_FAILURE);
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}