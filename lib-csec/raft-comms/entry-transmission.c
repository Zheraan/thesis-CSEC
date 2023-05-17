//
// Created by zheraan on 06/05/23.
//

#include "entry-transmission.h"

entry_transmission_s *etr_new(const overseer_s *overseer,
                              enum message_type type,
                              const data_op_s *op,
                              uint32_t index,
                              uint32_t term,
                              enum entry_state state) {
    entry_transmission_s *ntr = malloc(sizeof(entry_transmission_s));
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

entry_transmission_s *etr_new_from_local_entry(const overseer_s *overseer,
                                               enum message_type type,
                                               uint64_t entry_id) {
    // Seek entry with given id
    log_entry_s *target_entry = log_get_entry_by_id(overseer->log, entry_id);
    if (target_entry == NULL) {
        fprintf(stderr,
                "New ETR from local entry: requested latest entry but no entry has ID %ld\n",
                entry_id);
        return NULL;
    }

    // Create new ETR
    entry_transmission_s *netr = etr_new(overseer,
                                         type,
                                         &(target_entry->op),
                                         overseer->log->next_index - 1,
                                         target_entry->term,
                                         target_entry->state);

    return netr;
}


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

int etr_reception_init(overseer_s *overseer) {
    debug_log(4, stdout, "- Initializing next entry transmission reception event ... ");
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->socket_etr,
                                              EV_READ,
                                              etr_receive_cb,
                                              (void *) overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Fatal error: failed to create the next ETR reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Message reception has low priority
    event_priority_set(reception_event, 1);

    if (overseer->etr_reception_event != NULL) // Freeing the past event if any
        event_free(overseer->etr_reception_event);
    overseer->etr_reception_event = reception_event;

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next ETR reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void etr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout, "Start of ETR reception callback ---------------------------------------------------\n");

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
            debug_log(1, stdout, "-> is ETR NEW\n");

            // TODO Effects of ETR NEW reception
            break;

        case MSG_TYPE_ETR_COMMIT:
            debug_log(1, stdout, "-> is ETR COMMIT\n");
            // TODO Effects of ETR COMMIT reception
            break;

        case MSG_TYPE_ETR_PROPOSITION:
            debug_log(1, stdout, "-> is ETR PROPOSITION\n");
            // If local node isn't P, send correction on who is P
            if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status != HOST_STATUS_P) {
                debug_log(1, stdout, "Local node isn't P, answering with INDICATE P ... ");
                if (cm_sendto(((overseer_s *) arg),
                              sender,
                              sender_len,
                              MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to Ack heartbeat\n");
                    return;
                }
                debug_log(1, stdout, "Done.\n");
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
            debug_log(1, stdout, "-> is ETR LOG FIX\n");
            // TODO Effects of ETR LOG FIX reception
            break;

        default:
            fprintf(stderr, "Invalid transmission type %d\n", tr.cm.type);
    }

    // Init next event so it can keep receiving messages
    etr_reception_init((overseer_s *) arg);

    debug_log(4, stdout, "End of TR reception callback ------------------------------------------------------\n");
    return;
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