//
// Created by zheraan on 06/05/23.
//

#include "entry-transmission.h"

void tr_print(const transmission_s *tr, FILE *stream) {
    cm_print(&(tr->cm), stream);
    fprintf(stream,
            "state: %d\n"
            "index: %d\n"
            "term:  %d\n"
            "data_op:\n"
            "- column   %ld\n"
            "- row      %ld\n"
            "- newval   %d\n",
            tr->state,
            tr->index,
            tr->term,
            tr->op.column,
            tr->op.row,
            tr->op.newval);
    return;
}

int tr_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, transmission_s *tr) {
    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf("Sending to %s the following TR:\n", buf);
        tr_print(tr, stdout);
    }

    do {
        errno = 0;
        if (sendto(overseer->socket_tr,
                   tr,
                   sizeof(transmission_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1) {
            perror("TR sendto");
            return EXIT_FAILURE;
        }
    } while (errno == EAGAIN);

    return EXIT_SUCCESS;
}

void tr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    transmission_s tr;
    struct sockaddr_in6 sender;
    socklen_t sender_len;

    do {
        errno = 0;
        if (recvfrom(fd, &tr, sizeof(transmission_s), 0, (struct sockaddr *) &sender, &sender_len) == -1)
            perror("recvfrom TR");
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
        printf("Received from %s a TR:\n", buf);
        if (DEBUG_LEVEL >= 3)
            tr_print(&tr, stdout);
    }

    // Responses depending on the type of transmission
    switch (tr.cm.type) {
        case MSG_TYPE_TR_ENTRY :
            if (DEBUG_LEVEL >= 1)
                printf("-> is TR ENTRY\n");
            // TODO Effects of TR ENTRY reception
            break;

        case MSG_TYPE_TR_COMMIT:
            if (DEBUG_LEVEL >= 1)
                printf("-> is TR COMMIT\n");
            // TODO Effects of TR COMMIT reception
            break;

        case MSG_TYPE_TR_PROPOSITION:
            if (DEBUG_LEVEL >= 1)
                printf("-> is TR PROPOSITION\n");
            // If local node isn't P, send correction on who is P
            if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status != HOST_STATUS_P) {
                fprintf(stderr, "Invalid transmission type %d\n", tr.cm.type);
                cm_sendto((overseer_s *) arg, sender, sender_len, MSG_TYPE_INDICATE_P);
                return;
            }

            if (log_add_entry((overseer_s *) arg, &tr, ENTRY_STATE_PENDING) == EXIT_FAILURE) {
                fprintf(stderr, "Failed to add a new log entry from proposition\n");
                return;
            }
            break;

        default:
            fprintf(stderr, "Invalid transmission type %d\n", tr.cm.type);
    }

    return;
}

transmission_s *tr_new(const overseer_s *overseer,
                       enum message_type type,
                       const data_op_s *op,
                       uint32_t index,
                       uint32_t term,
                       enum entry_state state) {
    transmission_s *ntr = malloc(sizeof(transmission_s));
    if (ntr == NULL) {
        perror("malloc new transmission struct");
        return NULL;
    }

    control_message_s *ncm = cm_new(overseer, type); // TODO Error handling
    ntr->cm = *ncm; // Copy the contents of the struct as transmissions can't contain pointers
    free(ncm);

    ntr->op = *op;
    ntr->term = term;
    ntr->index = index;
    ntr->state = state;
    return ntr;
}