//
// Created by zheraan on 22/04/23.
//

#include "master-events.h"

void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg) {
    // TODO Implement flags
    heartbeat_s *hb = heartbeat_new((overseer_s *) arg, MSG_TYPE_HB_DEFAULT);
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

        receiver = (target->addr);
        receiver_len = (target->addr_len);

        evutil_inet_ntop(AF_INET6, &(receiver.sin6_addr), buf, 256);
        if (DEBUG_LEVEL >= 2) {
            printf("Sending to %s (Status: %d) the following heartbeat:\n", buf, target->status);
            hb_print(hb, stdout);
        }

        do {
            errno = 0;
            if (sendto(sender,
                       hb,
                       sizeof(heartbeat_s),
                       0,
                       (const struct sockaddr *) &receiver,
                       receiver_len) == -1)
                perror("sendto");
        } while (errno == EAGAIN);

        // TODO Add ack timeout
    }

    free(hb);
    return;
}

int master_heartbeat_init(overseer_s *overseer) {
    // Create the event related to the socket
    struct event *sender_event = event_new(overseer->eb,
                                           overseer->socket_hb,
                                           EV_PERSIST | EV_TIMEOUT,
                                           master_heartbeat_broadcast_cb,
                                           (void *) overseer);
    if (sender_event == NULL) {
        fprintf(stderr, "Failed to create the heartbeat event\n");
        return (EXIT_FAILURE);
    }

    // Heartbeat has high priority
    event_priority_set(sender_event, 0);

    if (event_list_add(overseer, sender_event) == EXIT_FAILURE) {
        fprintf(stderr, "Failed to allocate the event list struct for heartbeat event\n");
        return (EXIT_FAILURE);
    }

    struct timeval sender_timeout = {
            .tv_sec = MASTER_HEARTBEAT_TIMEOUT_SEC,
            .tv_usec = MASTER_HEARTBEAT_TIMEOUT_USEC
    };
    // Add the event in the loop
    if (event_add(sender_event, &sender_timeout) != 0) {
        fprintf(stderr, "Failed to add the heartbeat event\n");
        return (EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

