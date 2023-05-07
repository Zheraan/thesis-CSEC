//
// Created by zheraan on 26/04/23.
//

#include "heartbeat.h"

void hb_print(heartbeat_s *hb, FILE *stream) {
    fprintf(stream,
            "host_id:       %d\n"
            "status:        %d\n"
            "type:         %d\n"
            "next_index:    %ld\n"
            "rep_index:     %ld\n"
            "match_index:   %ld\n"
            "term:          %d\n",
            hb->host_id,
            hb->status,
            hb->type,
            hb->next_index,
            hb->rep_index,
            hb->match_index,
            hb->term);
    return;
}

void heartbeat_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type) {
    heartbeat_s *hb = heartbeat_new(overseer, type);

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    do {
        errno = 0;
        if (sendto(overseer->socket_hb,
                   hb,
                   sizeof(heartbeat_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1)
            perror("heartbeat sendto");
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        printf("Sending to %s the following heartbeat:\n", buf);
        hb_print(hb, stdout);
    }

    free(hb);
    return;
}

void heartbeat_receive_cb(evutil_socket_t fd, short event, void *arg) {
    heartbeat_s hb;
    struct sockaddr_in6 sender;
    socklen_t sender_len;

    do {
        errno = 0;
        if (recvfrom(fd, &hb, sizeof(heartbeat_s), 0, (struct sockaddr *) &sender, &sender_len) == -1)
            perror("recvfrom");
    } while (errno == EAGAIN);

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
    if (DEBUG_LEVEL >= 1) {
        printf("Received from %s a heartbeat:\n", buf);
        if (DEBUG_LEVEL >= 3)
            hb_print(&hb, stdout);
    }

    // Responses depending on the type of heartbeat
    if (hb.type == MSG_TYPE_HB_DEFAULT) {
        // Default heartbeat means replying with hb ack
        if (DEBUG_LEVEL >= 1)
            printf("-> is HB DEFAULT\n");
        heartbeat_sendto(((overseer_s *) arg), sender, sender_len, MSG_TYPE_ACK_HB);
    }
    if (hb.type == MSG_TYPE_ACK_HB) {
        if (DEBUG_LEVEL >= 1)
            printf("-> is HB ACK\n");
        // TODO Apply any effects of ack (reset timeout for next ack...)
    }
    if (hb.type == MSG_TYPE_P_TAKEOVER) {
        if (DEBUG_LEVEL >= 1)
            printf("-> is P TAKEOVER\n");
        // TODO Check term and apply any effects of P takeover if valid
    }

    return;
}

heartbeat_s *heartbeat_new(overseer_s *overseer, enum message_type type) {
    heartbeat_s *nhb = malloc(sizeof(heartbeat_s));

    if (nhb == NULL) {
        perror("malloc new heartbeat struct");
        exit(EXIT_FAILURE);
    }

    nhb->host_id = overseer->hl->localhost_id;
    nhb->status = overseer->hl->hosts[nhb->host_id].status;
    nhb->type = type;
    nhb->next_index = overseer->log->next_index;
    nhb->rep_index = overseer->log->rep_index;
    nhb->match_index = overseer->log->match_index;
    nhb->term = overseer->log->P_term;

    return nhb;
}
