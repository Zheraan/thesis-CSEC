//
// Created by zheraan on 06/05/23.
//

#include "entry-transmission.h"

entry_transmission_s *etr_new(const overseer_s *overseer,
                              enum message_type type,
                              const data_op_s *op,
                              uint32_t index,
                              uint32_t term,
                              enum entry_state state,
                              uint32_t ack_back) {
    entry_transmission_s *netr = malloc(sizeof(entry_transmission_s));
    if (netr == NULL) {
        perror("malloc new transmission struct");
        return NULL;
    }

    control_message_s *ncm = cm_new(overseer, type, ack_back);
    if (ncm == NULL) {
        free(netr);
        return (NULL);
    }
    netr->cm = *ncm; // Copy the contents of the struct as transmissions can't contain pointers
    free(ncm);

    netr->op.newval = op->newval;
    netr->op.row = op->row;
    netr->op.column = op->column;
    netr->term = term;
    netr->index = index;
    netr->state = state;
    return netr;
}

entry_transmission_s *etr_new_from_local_entry(const overseer_s *overseer,
                                               enum message_type type,
                                               uint64_t entry_id,
                                               uint32_t ack_back) {
    // Seek entry with given id
    log_entry_s *target_entry = log_get_entry_by_id(overseer->log, entry_id);
    if (target_entry == NULL) {
        fprintf(stderr,
                "Error creating new ETR from local entry: no entry has ID %ld\n",
                entry_id);
        return NULL;
    }

    // Create new ETR
    entry_transmission_s *netr = etr_new(overseer,
                                         type,
                                         &(target_entry->op),
                                         entry_id,
                                         target_entry->term,
                                         target_entry->state,
                                         ack_back);
    return netr;
}

void etr_print(const entry_transmission_s *etr, FILE *stream) {
    cm_print(&(etr->cm), stream);
    fprintf(stream,
            "state:         %d\n"
            "index:         %d\n"
            "P-term:          %d\n"
            "data_op:\n"
            " - row:        %d\n"
            " - column:     %d\n"
            " - newval:     %c\n",
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
        if (rt_attempts > 0)
            printf("Sending ETR of type %d with %d retransmission attempt(s) ... \n", type, rt_attempts);
        else
            printf("Sending ETR of type %d ... \n", type);
        fflush(stdout);
    }

    // If there are no retransmissions attempts (and thus no need for an ack), the ack number is always 0. Otherwise,
    // we make sure that it's initialized with the cache entry's ID, or to not reset it if it has already been set as
    // freshly allocated CMs have an ack_reference of 0, unless it is set when creating the related retransmission event,
    // like here.
    if (rt_attempts == 0 && etr->cm.ack_reference == 0)
        etr->cm.ack_reference = 0;
    else if (etr->cm.ack_reference == 0) {
        uint32_t rv = rt_cache_add_new(overseer,
                                       rt_attempts,
                                       sockaddr,
                                       socklen,
                                       type,
                                       etr,
                                       etr->cm.ack_back);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        etr->cm.ack_reference = rv;

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
            if (errno != EAGAIN)
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

    entry_transmission_s etr;
    struct sockaddr_in6 sender;
    socklen_t sender_len = sizeof(sender);

    do {
        errno = 0;
        if (recvfrom(fd, &etr, sizeof(entry_transmission_s), 0, (struct sockaddr *) &sender, &sender_len) == -1) {
            perror("recvfrom ETR");
            if (errno != EAGAIN)
                return; // Failure
        }
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
        printf("Received from %s an ETR:\n", buf);
        if (DEBUG_LEVEL >= 3)
            etr_print(&etr, stdout);
    }


    // If the incoming message calls for an acknowledgement, we must set it the value for the next answer
    // TODO later check if there can be a case where a message calling for an ack can lead to a message
    //  being sent to another node, therefore possibly acknowledging the wrong message
    uint32_t ack_back = etr.cm.ack_reference;

    // If the incoming message is acknowledging a previously sent message, remove its retransmission cache
    if (etr.cm.ack_back != 0) {
        debug_log(4,
                  stdout,
                  "-> Ack back value is non-zero, removing corresponding RT cache entry ... ");
        if (rt_cache_remove_by_id((overseer_s *) arg, etr.cm.ack_back) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");
        else debug_log(4, stderr, "Failure.\n");
    }

    enum cm_check_rv crv = cm_check_metadata((overseer_s *) arg, &(etr.cm));
    if (crv == CHECK_RV_FAILURE) {
        // Cause of failure should be printed to stderr by the function
        debug_log(4, stdout,
                  "End of ETR reception callback ------------------------------------------------------\n\n");
        return;
    }
    cm_check_action((overseer_s *) arg, crv, sender, sender_len, &(etr.cm)); // TODO Failure handling

    // Responses depending on the type of transmission
    switch (etr.cm.type) {
        case MSG_TYPE_ETR_NEW : // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status == HOST_STATUS_P) {
                // TODO Edgecase: reception of new entry as P
                debug_log(1, stdout, "-> is ETR NEW, but local is P, aborting ...\n");
                debug_log(4, stdout,
                          "End of ETR reception callback ------------------------------------------------------\n\n");
                return;
            }

            debug_log(1, stdout, "-> is ETR NEW\n");
            if (crv == CHECK_RV_CLEAN)
                log_add_entry((overseer_s *) arg, &etr, etr.state);
            else if (crv == CHECK_RV_NEED_REPLAY || crv == CHECK_RV_NEED_REPAIR)
                log_add_entry((overseer_s *) arg, &etr, ENTRY_STATE_CACHED);

            // TODO Effects of ETR NEW reception
            break;

        case MSG_TYPE_ETR_COMMIT: // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            debug_log(1, stdout, "-> is ETR COMMIT\n");
            // TODO Effects of ETR COMMIT reception
            break;

        case MSG_TYPE_ETR_PROPOSITION: // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            debug_log(1, stdout, "-> is ETR PROPOSITION\n");
            // If local node isn't P, send correction on who is P
            if (((overseer_s *) arg)->hl->hosts[((overseer_s *) arg)->hl->localhost_id].status != HOST_STATUS_P) {
                debug_log(1, stdout, "Local node isn't P, answering with INDICATE P ... ");
                if (cm_sendto_with_rt_init(((overseer_s *) arg),
                                           sender,
                                           sender_len,
                                           MSG_TYPE_INDICATE_P,
                                           0,
                                           0,
                                           ack_back) != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to Ack heartbeat\n");
                    return;
                }
                debug_log(1, stdout, "Done.\n");
                return;
            }
            // Else local is P:
            // add entry to the log
            if (log_add_entry((overseer_s *) arg, &etr, ENTRY_STATE_PENDING) != EXIT_SUCCESS) {
                fprintf(stderr, "Failed to add a new log entry from proposition\n");
                return;
            }
            // TODO Sync the proposition with other nodes
            break;

        case MSG_TYPE_ETR_LOGFIX: // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            debug_log(1, stdout, "-> is ETR LOG FIX\n");
            // TODO Effects of ETR LOG FIX reception
            break;

        case MSG_TYPE_ETR_NEW_AND_ACK: // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            debug_log(1, stdout, "-> is ETR NEW AND ACK\n");
            // TODO Effects of ETR NEW AND ACK reception
            break;

        default:
            fprintf(stderr, "Invalid transmission type %d\n", etr.cm.type);
    }

    // Init next event so it can keep receiving messages
    etr_reception_init((overseer_s *) arg);

    debug_log(4, stdout, "End of ETR reception callback ------------------------------------------------------\n\n");
    return;
}

void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("ETR retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((retransmission_cache_s *) arg)->cur_attempts + 1);
        fflush(stdout);
    }
    int success = 1;

    // Update the control message in the transmission, as program status can change between transmissions attempts.
    control_message_s *ncm = cm_new(((retransmission_cache_s *) arg)->overseer,
                                    ((retransmission_cache_s *) arg)->etr->cm.type,
                                    ((retransmission_cache_s *) arg)->ack_back);
    ((retransmission_cache_s *) arg)->etr->cm = *ncm;
    free(ncm);

    // Send proposition to P
    if (etr_sendto(((retransmission_cache_s *) arg)->overseer,
                   ((retransmission_cache_s *) arg)->addr,
                   ((retransmission_cache_s *) arg)->socklen,
                   ((retransmission_cache_s *) arg)->etr)) {
        fprintf(stderr, "Failed retransmitting ETR\n");
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
            fprintf(stderr, "Failed to add the ETR retransmission event\n");
            fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    return;
}