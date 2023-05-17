//
// Created by zheraan on 06/05/23.
//

#include "entry-transmission.h"

void etr_print(const entry_transmission_s *etr, FILE *stream) {
    cm_print(&(etr->cm), stream);
    fprintf(stream,
            "state: %d\n"
            "index: %d\n"
            "term:  %d\n"
            "data_op:\n"
            "- row      %d\n"
            "- column   %d\n"
            "- newval   %c\n",
            etr->state,
            etr->index,
            etr->term,
            etr->op.row,
            etr->op.column,
            etr->op.newval);
    return;
}

int etr_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, entry_transmission_s *etr) {
    return etr_sendto_with_rt_init(overseer, etr, sockaddr, socklen, etr->cm.type, 0);
}

int etr_sendto_with_rt_init(overseer_s *overseer,
                            entry_transmission_s *etr,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            enum message_type type,
                            uint8_t rt_attempts) {

    if (DEBUG_LEVEL >= 3) {
        printf("Sending ETR of type %d with %d retransmission rt_attempts ... \n", type, rt_attempts);
        fflush(stdout);
    }

    if (rt_attempts == 0)
        etr->cm.ack_number = 0;
    else {
        uint32_t rv = rt_cache_add_new(overseer, rt_attempts, sockaddr, socklen, type, etr);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        etr->cm.ack_number = rv;

        debug_log(4, stdout, " - ETR RT init OK\n");
    }

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf("Sending to %s the following ETR:\n", buf);
        etr_print(etr, stdout);
    }
    // Ensuring we are sending to the transmission port, since the stored addresses in the hosts-list contain the right
    // address but with the Control Message port (35007)
    sockaddr.sin6_port = htons(35008);

    do {
        errno = 0;
        if (sendto(overseer->socket_etr,
                   etr,
                   sizeof(entry_transmission_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1) {
            perror("ETR sendto");
            return EXIT_FAILURE;
        }
    } while (errno == EAGAIN);

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void etr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 4) {
        printf("Start of ETR reception callback ---------------------------------------------------\n");
        fflush(stdout);
    }

    entry_transmission_s tr;
    struct sockaddr_in6 sender;
    socklen_t sender_len = sizeof(sender);

    do {
        errno = 0;
        if (recvfrom(fd, &tr, sizeof(entry_transmission_s), 0, (struct sockaddr *) &sender, &sender_len) == -1)
            perror("recvfrom TR");
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
        printf("Received from %s a TR:\n", buf);
        if (DEBUG_LEVEL >= 3)
            etr_print(&tr, stdout);
    }

    // Responses depending on the type of transmission
    switch (tr.cm.type) {
        case MSG_TYPE_ETR_NEW :
            if (DEBUG_LEVEL >= 1)
                printf("-> is ETR NEW\n");
            // TODO Effects of ETR NEW reception
            break;

        case MSG_TYPE_ETR_COMMIT:
            if (DEBUG_LEVEL >= 1)
                printf("-> is ETR COMMIT\n");
            // TODO Effects of ETR COMMIT reception
            break;

        case MSG_TYPE_ETR_PROPOSITION:
            if (DEBUG_LEVEL >= 1)
                printf("-> is ETR PROPOSITION\n");
            // If local node isn't P, send correction on who is P
            if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status != HOST_STATUS_P) {
                if (DEBUG_LEVEL >= 1)
                    printf("Local node isn't P, answering with INDICATE P ... ");
                if (cm_sendto(((overseer_s *) arg),
                              sender,
                              sender_len,
                              MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to Ack heartbeat\n");
                    return;
                }
                if (DEBUG_LEVEL >= 1)
                    printf("Done.\n");
                return;
            }
            // Else local is P:
            // add entry to the log
            if (log_add_entry((overseer_s *) arg, &tr, ENTRY_STATE_PENDING) != EXIT_SUCCESS) {
                fprintf(stderr, "Failed to add a new log entry from proposition\n");
                return;
            }
            // TODO Sync the proposition with other nodes
            break;

        case MSG_TYPE_ETR_LOGFIX:
            if (DEBUG_LEVEL >= 1)
                printf("-> is ETR LOG FIX\n");
            // TODO Effects of ETR LOG FIX reception
            break;

        default:
            fprintf(stderr, "Invalid transmission type %d\n", tr.cm.type);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("TR retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((retransmission_cache_s *) arg)->cur_attempts + 1);
        fflush(stdout);
    }
    int success = 1;

    // Update the control message in the transmission, as program status can change between transmissions attempts.
    control_message_s *ncm = cm_new(((retransmission_cache_s *) arg)->overseer,
                                    ((retransmission_cache_s *) arg)->etr->cm.type);
    ((retransmission_cache_s *) arg)->etr->cm = *ncm;
    free(ncm);

    // Send proposition to P
    if (etr_sendto(((retransmission_cache_s *) arg)->overseer,
                   ((retransmission_cache_s *) arg)->addr,
                   ((retransmission_cache_s *) arg)->socklen,
                   ((retransmission_cache_s *) arg)->etr)) {
        fprintf(stderr, "Failed retransmitting TR\n");
        success = 0;
    }

    // Increase attempts number
    ((retransmission_cache_s *) arg)->cur_attempts++;

    // If attempts max reached, remove cache entry
    if (((retransmission_cache_s *) arg)->cur_attempts >= ((retransmission_cache_s *) arg)->max_attempts) {
        rt_cache_remove_by_id(((retransmission_cache_s *) arg)->overseer, ((retransmission_cache_s *) arg)->id);
    } else { // Otherwise add retransmission event
        // Add the event in the loop
        struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
        if (errno == EUNKNOWN_TIMEOUT_TYPE ||
            event_add(((retransmission_cache_s *) arg)->ev, &ops_timeout) != 0) {
            fprintf(stderr, "Failed to add the TR retransmission event\n");
            fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    return;
}