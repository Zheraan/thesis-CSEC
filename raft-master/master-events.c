//
// Created by zheraan on 22/04/23.
//

#include "master-events.h"

void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg) {
    debug_log(4, stdout, "Start of HB broadcast callback ----------------------------------------------------\n");
    debug_log(3, stdout, "Broadcasting heartbeat ... ");

    host_s *target;
    enum host_status local_status = (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id]).status;
    uint32_t nb_hosts = ((overseer_s *) arg)->hl->nb_hosts;
    struct sockaddr_in6 receiver;
    socklen_t receiver_len;
    char buf[256];

    int nb_heartbeats = 0;
    for (uint32_t i = 0; i < nb_hosts; i++) {
        target = &(((overseer_s *) arg)->hl->hosts[i]);

        // TODO Extension Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip iteration if local is P and target is a CS node
        if (local_status == HOST_STATUS_P && target->status == HOST_STATUS_CS)
            continue;

        // Skip iteration if local is HS and target isn't a master node
        if (local_status == HOST_STATUS_HS && target->type != NODE_TYPE_M)
            continue;

        // Skip if target is the local host
        if (target->locality == HOST_LOCALITY_LOCAL)
            continue;

        if (DEBUG_LEVEL >= 4) {
            printf("\n- Heartbeat target: %s\n", target->name);
            fflush(stdout);
        }

        receiver = (target->addr);
        receiver_len = (target->socklen);

        evutil_inet_ntop(AF_INET6, &(receiver.sin6_addr), buf, 256);
        if (DEBUG_LEVEL >= 3) {
            printf("- Heartbeat: ");
        }

        if (cm_sendto_with_rt_init(((overseer_s *) arg),
                                   receiver,
                                   receiver_len,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   0) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to send and RT init heartbeat\n");
        } else nb_heartbeats++;
    }

    // Set the next event
    if (master_heartbeat_init((overseer_s *) arg) != EXIT_SUCCESS) {
        fprintf(stderr, "Fatal Error: next heartbeat event couldn't be set\n");
        fflush(stderr);
        exit(EXIT_FAILURE); // TODO Extension Crash handler
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d heartbeats sent).\n", nb_heartbeats);
        fflush(stdout);
    }
    debug_log(4, stdout, "End of HB broadcast callback ------------------------------------------------------\n\n");
    return;
}

int master_heartbeat_init(overseer_s *overseer) {
    debug_log(3, stdout, "\n- Initializing next heartbeat event ... ");

    // Create the event related to the socket
    struct event *hb_event = event_new(overseer->eb,
                                       overseer->socket_cm,
                                       EV_TIMEOUT,
                                       master_heartbeat_broadcast_cb,
                                       (void *) overseer);
    if (hb_event == NULL) {
        fprintf(stderr, "Failed to create the heartbeat event\n");
        fflush(stderr);
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
        fflush(stderr);
        return (EXIT_FAILURE);
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

