//
// Created by zheraan on 22/04/23.
//

#include "master-events.h"

void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("Broadcasting heartbeat ... ");
        fflush(stdout);
    }
    host_s *target;
    host_s *local = &(((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id]);
    uint32_t nb_hosts = ((overseer_s *) arg)->hl->nb_hosts;
    struct sockaddr_in6 receiver;
    socklen_t receiver_len;
    char buf[256];

    for (int i = 0; i < nb_hosts; i++) {
        target = &(((overseer_s *) arg)->hl->hosts[i]);

        // TODO Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip iteration if local is P and target isn't either S or HS
        if (local->status == HOST_STATUS_P && (target->status != HOST_STATUS_S || target->status != HOST_STATUS_HS))
            continue;

        // Skip iteration if local is HS and target isn't CS
        if (local->status == HOST_STATUS_HS && target->status != HOST_STATUS_CS)
            continue;

        // Skip if target is the local host
        if (target->locality == HOST_LOCALITY_LOCAL)
            continue;

        receiver = (target->addr);
        receiver_len = (target->addr_len);

        evutil_inet_ntop(AF_INET6, &(receiver.sin6_addr), buf, 256);
        if (DEBUG_LEVEL >= 3) {
            printf("Heartbeat: ");
        }

        if (cm_sendto(((overseer_s *) arg),
                      receiver,
                      receiver_len,
                      MSG_TYPE_HB_DEFAULT) == EXIT_FAILURE) {
            fprintf(stderr, "Failed to send heartbeat\n");
            fflush(stderr);
            return;
        }

        // TODO Add ack timeout
    }

    if (master_heartbeat_init((overseer_s *) arg) == EXIT_FAILURE) {
        fprintf(stderr, "Fatal Error: heartbeat event couldn't be set\n");
        fflush(stderr);
        exit(EXIT_FAILURE); // TODO Crash handler
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n");
        fflush(stdout);
    }
    return;
}

int master_heartbeat_init(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 3) {
        printf("- Initializing heartbeat events ... ");
        fflush(stdout);
    }
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

    if (overseer->hb_event != NULL) // Freeing the past heartbeat if any
        event_free(overseer->hb_event);
    overseer->hb_event = hb_event;

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

    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

