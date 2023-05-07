//
// Created by zheraan on 06/05/23.
//

#include "entry-transmission.h"

void tr_print(const transmission_s *tr, FILE *stream) {
    // TODO
    return;
}

void tr_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, transmission_s *tr) {
    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    do {
        errno = 0;
        if (sendto(overseer->socket_tr,
                   tr,
                   sizeof(transmission_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1)
            perror("TR sendto");
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 3) {
        printf("Sending to %s the following TR:\n", buf);
        tr_print(tr, stdout);
    }

    free(tr);
    return;
}

void tr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    // TODO tr_receive_cb
    return;
}

transmission_s *tr_new(const overseer_s *overseer,
                       enum message_type type,
                       data_op_s op,
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

    ntr->op = op;
    ntr->term = term;
    ntr->index = index;
    ntr->state = state;
    return ntr;
}