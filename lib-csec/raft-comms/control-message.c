//
// Created by zheraan on 26/04/23.
//

#include "control-message.h"

void cm_print(const control_message_s *hb, FILE *stream) {
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

void cm_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type) {
    control_message_s *cm = cm_new(overseer, type);

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    do {
        errno = 0;
        if (sendto(overseer->socket_cm,
                   cm,
                   sizeof(control_message_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1)
            perror("cm sendto");
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        printf("Sending to %s the following cm:\n", buf);
        cm_print(cm, stdout);
    }

    free(cm);
    return;
}

void cm_receive_cb(evutil_socket_t fd, short event, void *arg) {
    control_message_s cm;
    struct sockaddr_in6 sender;
    socklen_t sender_len;

    do {
        errno = 0;
        if (recvfrom(fd, &cm, sizeof(control_message_s), 0, (struct sockaddr *) &sender, &sender_len) == -1)
            perror("recvfrom");
    } while (errno == EAGAIN);

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
    if (DEBUG_LEVEL >= 1) {
        printf("Received from %s a cm:\n", buf);
        if (DEBUG_LEVEL >= 3)
            cm_print(&cm, stdout);
    }

    // Responses depending on the type of control message
    switch (cm.type) {
        case MSG_TYPE_HB_DEFAULT :
            if (DEBUG_LEVEL >= 1)
                printf("-> is HB DEFAULT\n");
            // Default heartbeat means replying with cm ack
            cm_sendto(((overseer_s *) arg), sender, sender_len, MSG_TYPE_ACK_HB);
            break;

        case MSG_TYPE_ACK_HB:
            if (DEBUG_LEVEL >= 1)
                printf("-> is HB ACK\n");
            // TODO Apply any effects of ack (reset timeout for next ack...)
            break;

        case MSG_TYPE_P_TAKEOVER:
            if (DEBUG_LEVEL >= 1)
                printf("-> is P TAKEOVER\n");
            // TODO Check term and apply any effects of P takeover if valid
            break;

            // TODO Implement other message types
        default:
            fprintf(stderr, "Unknown control message type %d\n", cm.type);
    }

    return;
}

control_message_s *cm_new(const overseer_s *overseer, enum message_type type) {
    control_message_s *ncm = malloc(sizeof(control_message_s));

    if (ncm == NULL) {
        perror("malloc new control message struct");
        return(NULL);
    }

    ncm->host_id = overseer->hl->localhost_id;
    ncm->status = overseer->hl->hosts[ncm->host_id].status;
    ncm->type = type;
    ncm->next_index = overseer->log->next_index;
    ncm->rep_index = overseer->log->rep_index;
    ncm->match_index = overseer->log->match_index;
    ncm->term = overseer->log->P_term;

    return ncm;
}
