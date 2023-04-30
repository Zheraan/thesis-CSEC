//
// Created by zheraan on 22/04/23.
//

#include "master-events.h"

void heartbeat_broadcast(evutil_socket_t sender, short event, void *arg) {
    heartbeat_s *hb = heartbeat_new((overseer_s *) arg);
    host_s *target;
    host_s *local = &(((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id]);
    uint32_t nb_hosts = ((overseer_s *) arg)->hl->nb_hosts;
    struct sockaddr_in6 receiver;
    socklen_t receiver_len;
    char buf[256];

    for (int i = 0; i < nb_hosts; i++) {
        target = &(((overseer_s *) arg)->hl->hosts[i]);

        // Skip iteration if local is P and target isn't either S or HS
        if (local->status == HOST_STATUS_P && (target->status != HOST_STATUS_S || target->status != HOST_STATUS_HS))
            continue;

        // Skip iteration if local is HS and target isn't CS
        if (local->status  == HOST_STATUS_HS && target->status != HOST_STATUS_CS)
            continue;

        receiver = (target->addr);
        receiver_len = (target->addr_len);

        evutil_inet_ntop(AF_INET6, &(receiver.sin6_addr), buf, 256);
        if (DEBUG_LEVEL >= 2) {
            printf("Sending to %s (Status: %d) the following heartbeat:\n", buf, target->status);
            print_hb(hb, stdout);
        }

        if (sendto(sender, hb, sizeof(heartbeat_s), 0, &receiver, receiver_len) == -1)
            perror("sendto");
    }

    free(hb);
    return;
}