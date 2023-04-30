//
// Created by zheraan on 26/04/23.
//

#include "heartbeat.h"

void hb_print(heartbeat_s *hb, FILE *stream) {
    fprintf(stream, "host_id:       %d\n"
                    "status:        %d\n"
                    "flags:         %d\n"
                    "next_index:    %ld\n"
                    "rep_index:     %ld\n"
                    "match_index:   %ld\n"
                    "commit_index:  %ld\n"
                    "term:          %d\n\n",
            hb->host_id,
            hb->status,
            hb->flags,
            hb->next_index,
            hb->rep_index,
            hb->match_index,
            hb->commit_index,
            hb->term);
    return;
}

void heartbeat_sendto(evutil_socket_t sender, short event, void *arg) {
    heartbeat_s *hb = heartbeat_new((overseer_s *) arg, 0);

    // FIXME change to include target of heartbeat instead of hosts[1]
    struct sockaddr_in6 receiver = (((overseer_s *) arg)->hl->hosts[1].addr);
    socklen_t receiver_len = (((overseer_s *) arg)->hl->hosts[1].addr_len);

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(receiver.sin6_addr), buf, 256);
    if (DEBUG_LEVEL > 2) {
        printf("Sending to %s the following heartbeat:\n", buf);
        hb_print(hb, stdout);
    }

    if (sendto(sender,
               hb,
               sizeof(heartbeat_s),
               0,
               (const struct sockaddr *) &receiver,
               receiver_len) == -1)
        perror("sendto");

    // FIXME replace message counter
    if (++message_counter >= 3) {
        event_base_loopbreak(((overseer_s *) arg)->eb);
    }

    free(hb);
    return;
}

void heartbeat_receive(evutil_socket_t listener, short event, void *arg) {
    heartbeat_s hb;
    struct sockaddr_in6 sender;
    socklen_t sender_len;

    if (recvfrom(listener, &hb, sizeof(heartbeat_s), 0, (struct sockaddr *) &sender, &sender_len) == -1)
        perror("recvfrom");

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
    printf("Received from %s the following heartbeat:\n", buf);
    hb_print(&hb, stdout);

    if (++message_counter >= 3) {
        event_base_loopbreak(((overseer_s *) arg)->eb);
    }
    return;
}


heartbeat_s *heartbeat_new(overseer_s *overseer, int flags) {
    heartbeat_s *nhb = malloc(sizeof(heartbeat_s));

    if (nhb == NULL) {
        perror("malloc new heartbeat struct");
        exit(EXIT_FAILURE);
    }

    nhb->host_id = overseer->hl->localhost_id;
    nhb->status = overseer->hl->hosts[nhb->host_id].status;
    nhb->flags = flags;
    nhb->next_index = overseer->log->next_index;
    nhb->rep_index = overseer->log->rep_index;
    nhb->match_index = overseer->log->match_index;
    nhb->commit_index = overseer->log->commit_index;
    nhb->term = overseer->log->P_term;

    return nhb;
}