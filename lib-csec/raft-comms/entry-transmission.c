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
    netr->prev_term = index > 1 ? overseer->log->entries[index - 1].term : 0;
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
                "Error creating new ETR from local entry: no entry has ID %ld.\n",
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
    fprintf(stream, "- CM metadata:\n");
    cm_print(&(etr->cm), stream);
    fprintf(stream,
            "- Entry Metadata:\n"
            "   > state:         %d\n"
            "   > index:         %d\n"
            "   > P-term:        %d\n"
            "- Data Op Metadata:\n"
            "   > row:           %d\n"
            "   > column:        %d\n"
            "   > newval:        %c\n",
            etr->state,
            etr->index,
            etr->term,
            etr->op.row,
            etr->op.column,
            etr->op.newval);
    return;
}

int etr_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, entry_transmission_s *etr) {
    return etr_sendto_with_rt_init(overseer, sockaddr, socklen, etr, 0);
}

int etr_sendto_with_rt_init(overseer_s *overseer,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            entry_transmission_s *etr,
                            uint8_t rt_attempts) {
    if (DEBUG_LEVEL >= 3) {
        if (rt_attempts > 0)
            printf("Sending ETR of type %d with %d retransmission attempt(s) ... \n", etr->cm.type, rt_attempts);
        else
            printf("Sending ETR of type %d ... \n", etr->cm.type);
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    // If there are no retransmissions attempts (and thus no need for an ack), the ack number is always 0. Otherwise,
    // we make sure that it's initialized with the cache entry's ID, or to not reset it if it has already been set as
    // freshly allocated CMs have an ack_reference of 0, unless it is set when creating the related retransmission event,
    // like here.
    if (rt_attempts == 0 && etr->cm.ack_reference == 0)
        etr->cm.ack_reference = 0;
    else if (etr->cm.ack_reference == 0) {
        uint32_t rv = rtc_add_new(overseer,
                                  rt_attempts,
                                  sockaddr,
                                  socklen,
                                  etr->cm.type,
                                  etr,
                                  etr->cm.ack_back);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache.\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            return EXIT_FAILURE;
        }
        etr->cm.ack_reference = rv;

        debug_log(4, stdout, " - ETR RT init OK.\n");
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

    if (FUZZER_ENABLED) {
        fuzzer_entry_init(overseer, PACKET_TYPE_ETR, (union packet) *etr, sockaddr, socklen);
    } else {
        // If the fuzzer is disabled, send it normally
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
    }

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
        if (INSTANT_FFLUSH) fflush(stderr);
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
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void etr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout, "Start of ETR reception callback ---------------------------------------------------\n");

    overseer_s *overseer = arg;
    entry_transmission_s etr;
    struct sockaddr_in6 sender_addr;
    socklen_t socklen = sizeof(sender_addr);

    do {
        errno = 0;
        if (recvfrom(fd,
                     &etr,
                     sizeof(entry_transmission_s),
                     0,
                     (struct sockaddr *) &sender_addr,
                     &socklen) == -1) {
            perror("recvfrom ETR");
            if (errno != EAGAIN)
                return; // Failure
        }
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 1) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender_addr.sin6_addr), buf, 256);
        printf("Received from %s (aka. %s)  an ETR:\n", buf, overseer->hl->hosts[etr.cm.host_id].name);
        if (DEBUG_LEVEL >= 3)
            etr_print(&etr, stdout);
    }

    // If the incoming message is acknowledging a previously sent message, remove its retransmission cache
    if (etr.cm.ack_back != 0) {
        debug_log(4,
                  stdout,
                  "-> Ack back value is non-zero, removing corresponding RT cache entry ... ");
        if (rtc_remove_by_id(overseer, etr.cm.ack_back, FLAG_DEFAULT) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");
        else
            debug_log(4,
                      stdout,
                      "Failure. The entry may have been removed earlier due to timeout.\n");
        // TODO Improvement: Log an error in case time was lower than timeout somehow or figure a way
    }

    if (DEBUG_LEVEL >= 2)
        cm_print_type(&(etr.cm), stdout);

    if (etr_actions(overseer, sender_addr, socklen, &etr) != EXIT_SUCCESS)
        debug_log(1, stderr, "ETR action failure.\n");

    // Init next event so it can keep receiving messages
    etr_reception_init(overseer);

    debug_log(4, stdout, "End of ETR reception callback ------------------------------------------------------\n\n");
    return;
}

void etr_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    retransmission_cache_s *rc = arg;

    if (DEBUG_LEVEL >= 3) {
        printf("ETR retransmission timed out, reattempting transmission (attempt %d) ... ",
               rc->cur_attempts + 1);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    int success = 1;

    // Update the control message in the transmission, as program status can change between transmissions attempts.
    control_message_s *ncm = cm_new(rc->overseer, rc->etr->cm.type, rc->ack_back);
    rc->etr->cm = *ncm;
    free(ncm);

    // Send proposition to P
    if (etr_sendto(rc->overseer, rc->addr, rc->socklen, rc->etr)) {
        fprintf(stderr, "Failed retransmitting ETR.\n");
        success = 0;
    }

    // Increase attempts number
    rc->cur_attempts++;

    // If attempts max reached, remove cache entry
    if (rc->cur_attempts >= rc->max_attempts) {
        rtc_remove_by_id(rc->overseer, rc->id, FLAG_DEFAULT);
    } else { // Otherwise add retransmission event
        // Add the event in the loop
        struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
        if (errno == EUNKNOWN_TIMEOUT_TYPE ||
            event_add(rc->ev, &ops_timeout) != 0) {
            fprintf(stderr, "Failed to add the ETR retransmission event.\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    return;
}

int etr_reply_logfix(overseer_s *overseer, const control_message_s *cm) {
    entry_transmission_s *netr = etr_new_from_local_entry(overseer,
                                                          MSG_TYPE_ETR_LOGFIX,
                                                          cm->next_index,
                                                          cm->ack_reference);
    if (etr_sendto_with_rt_init(overseer,
                                overseer->hl->hosts[cm->host_id].addr,
                                overseer->hl->hosts[cm->host_id].socklen,
                                netr,
                                ETR_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed to send and RT init a LOGFIX as reply.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int etr_broadcast_commit_order(overseer_s *overseer, uint64_t index) {
    if (overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P) {
        debug_log(0, stderr, "Error: only P can send a Commit Order.\n");
        return EXIT_FAILURE;
    }

    debug_log(4,
              stdout,
              "Start of Commit Order broadcast ----------------------------------------------------\n");

    entry_transmission_s *netr_blueprint = etr_new_from_local_entry(overseer,
                                                                    MSG_TYPE_ETR_COMMIT,
                                                                    index,
                                                                    0);
    if (netr_blueprint == NULL) {
        debug_log(0, stderr, "Failed to create a new ETR for sending commit order.\n");
        return EXIT_FAILURE;
    }

    host_s *target;
    struct sockaddr_in6 receiver;
    socklen_t receiver_len;

    debug_log(3, stdout, "Broadcasting Commit Order ... ");
    int nb_orders = 0;
    for (uint32_t i = 0; i < overseer->hl->nb_hosts; i++) {
        target = &(overseer->hl->hosts[i]);

        // TODO Extension Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip if target is the local host
        if (target->locality == HOST_LOCALITY_LOCAL)
            continue;

        if (DEBUG_LEVEL >= 4) {
            printf("\n- Order target: %s\n", target->name);
            if (INSTANT_FFLUSH) fflush(stdout);
        }

        receiver = (target->addr);
        receiver_len = (target->socklen);


        entry_transmission_s *netr = malloc(sizeof(entry_transmission_s));
        if (netr == NULL) {
            perror("Malloc new ETR for Entry Broadcast.\n");
            continue;
        }
        *netr = *netr_blueprint;

        if (DEBUG_LEVEL >= 3) {
            printf("- Commit order: ");
        }

        if (etr_sendto_with_rt_init(overseer,
                                    receiver,
                                    receiver_len,
                                    netr,
                                    ETR_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to send and RT init Commit Order.\n");
        } else nb_orders++;
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d orders sent).\n", nb_orders);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    free(netr_blueprint);
    debug_log(4,
              stdout,
              "End of Commit Order broadcast ------------------------------------------------------\n\n");
    return EXIT_SUCCESS;
}

int etr_broadcast_new_entry(overseer_s *overseer, uint64_t index, uint32_t sender_id, uint32_t ack_back) {
    debug_log(4, stdout, "Start of New Entry broadcast ----------------------------------------------------\n");

    entry_transmission_s *netr_noack = etr_new_from_local_entry(overseer,
                                                                MSG_TYPE_ETR_NEW,
                                                                index,
                                                                0);
    if (netr_noack == NULL) {
        debug_log(0, stderr, "Failed to create a new ETR for sending New Entry.\n");
        return EXIT_FAILURE;
    }

    // With ack for the entry's sender
    entry_transmission_s *netr_ack = etr_new_from_local_entry(overseer,
                                                              MSG_TYPE_ETR_NEW_AND_ACK,
                                                              index,
                                                              ack_back);
    if (netr_noack == NULL) {
        debug_log(0, stderr, "Failed to create a new ETR for sending New Entry.\n");
        return EXIT_FAILURE;
    }

    host_s *target;

    debug_log(3, stdout, "Broadcasting New Entry ... ");
    int nb_etr = 0;
    for (uint32_t i = 0; i < overseer->hl->nb_hosts; i++) {
        target = &(overseer->hl->hosts[i]);

        // TODO Extension Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip if target is the local host
        if (target->locality == HOST_LOCALITY_LOCAL)
            continue;

        if (DEBUG_LEVEL >= 4) {
            printf("\n- ETR target: %s\n", target->name);
            if (INSTANT_FFLUSH) fflush(stdout);
        }

        entry_transmission_s *netr = malloc(sizeof(entry_transmission_s));
        if (netr == NULL) {
            perror("Malloc new ETR for Entry Broadcast.\n");
            continue;
        }
        *netr = i == sender_id ? *netr_ack : *netr_noack; // Copy data from the "blueprint" ETR

        if (DEBUG_LEVEL >= 3) {
            printf("- New Entry: ");
        }

        if (etr_sendto_with_rt_init(overseer,
                                    target->addr,
                                    target->socklen,
                                    netr,
                                    ETR_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to send and RT init New Entry.\n");
        } else nb_etr++;
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d ETR sent).\n", nb_etr);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    free(netr_noack);
    free(netr_ack);
    debug_log(4, stdout, "End of New Entry broadcast ------------------------------------------------------\n\n");
    return EXIT_SUCCESS;
}

int etr_actions(overseer_s *overseer,
                struct sockaddr_in6 sender_addr,
                socklen_t socklen,
                entry_transmission_s *etr) {
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    if ((local_status == HOST_STATUS_P || local_status == HOST_STATUS_HS) &&
        etr->cm.P_term == overseer->log->P_term &&
        etr->cm.type != MSG_TYPE_INDICATE_P &&
        etr->cm.type != MSG_TYPE_INDICATE_HS) {
        hl_host_index_change(overseer, etr->cm.host_id, etr->cm.next_index, etr->cm.commit_index);
    }

    // If message has superior or equal P-term and comes from P and local is not already P
    if (etr->cm.P_term >= overseer->log->P_term &&
        etr->cm.status == HOST_STATUS_P &&
        local_status != HOST_STATUS_P) {
        debug_log(3, stdout, "Message is from active P:\n");
        p_liveness_set_timeout(overseer); // Reset P liveness timer
    }
    // If message has superior or equal HS-term and comes from HS and local is not already HS
    if (etr->cm.HS_term >= overseer->log->HS_term &&
        etr->cm.status == HOST_STATUS_HS &&
        local_status != HOST_STATUS_HS) {
        debug_log(3, stdout, "Message is from active HS:\n");
        election_set_timeout(overseer); // Reset HS election timer
    }

    if (local_status == HOST_STATUS_P)
        return etr_actions_as_p(overseer, sender_addr, socklen, etr);
    else return etr_actions_as_s_hs_cs(overseer, sender_addr, socklen, etr);
}

int etr_actions_as_p(overseer_s *overseer,
                     struct sockaddr_in6 sender_addr,
                     socklen_t socklen,
                     entry_transmission_s *etr) {
    int rv = EXIT_SUCCESS;

    // If dist P-term is greater than local
    if (etr->cm.P_term > overseer->log->P_term) {
        hl_update_status(overseer, etr->cm.status, etr->cm.host_id);
        stepdown_to_cs(overseer);
        if (cm_sendto_with_rt_init(overseer,
                                   sender_addr,
                                   socklen,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   etr->cm.ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to reply with a DEFAULT HB after stepping down to CS.\n");
        return EXIT_SUCCESS;
    }

    // If local P-term is greater than dist
    if (etr->cm.P_term < overseer->log->P_term) {
        if (cm_sendto_with_rt_init(overseer,
                                   sender_addr,
                                   socklen,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   etr->cm.ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to reply with a DEFAULT HB.\n");
        return EXIT_SUCCESS;
    }

    // Else if P-terms are equal

    if (etr->cm.type == MSG_TYPE_ETR_PROPOSITION) {
        if (etr->cm.next_index > overseer->log->next_index) {
            if (cm_sendto_with_rt_init(overseer,
                                       sender_addr,
                                       socklen,
                                       MSG_TYPE_HB_DEFAULT,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       etr->cm.ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to reply with a DEFAULT HB.\n");
            return EXIT_SUCCESS;
        }

        if (etr->cm.next_index < overseer->log->next_index) {
            debug_log(0,
                      stderr,
                      "Fatal error: P cannot have a lower next index if P-terms are equal.\n");
            exit(EXIT_FAILURE);
        }

        // Else if next indexes are equal
        log_add_entry(overseer, etr, ENTRY_STATE_PENDING);
        if (etr_broadcast_new_entry(overseer, etr->index, etr->cm.host_id, etr->cm.ack_reference)
            != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to broadcast new entry.\n");
            rv = EXIT_FAILURE;
        }
        hl_update_status(overseer, etr->cm.status, etr->cm.host_id);

    } else {
        debug_log(0,
                  stderr,
                  "Fatal error: P cannot receive ETRs other than propositions if P-terms are equal.\n");
        exit(EXIT_FAILURE);
    }

    return rv;
}

int etr_actions_as_s_hs_cs(overseer_s *overseer,
                           struct sockaddr_in6 sender_addr,
                           socklen_t socklen,
                           entry_transmission_s *etr) {
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    // If dist P-term is greater than local
    if (etr->cm.P_term > overseer->log->P_term) {
        if (DEBUG_LEVEL >= 4) {
            printf("Dist P-term (%d) is greater than local (%d).\n", etr->cm.P_term, overseer->log->P_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        hl_update_status(overseer, etr->cm.status, etr->cm.host_id);
        return log_repair(overseer, &etr->cm);
    }

    // If local P-term is greater than dist
    if (etr->cm.P_term < overseer->log->P_term) {
        if (DEBUG_LEVEL >= 4) {
            printf("Local P-term (%d) is greater than dist (%d).\n", overseer->log->P_term, etr->cm.P_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_INDICATE_P,
                                    etr->cm.ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to reply with an INDICATE P.\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // Else if P-terms are equal

    // If local not S and dist HS-term is greater than local
    if (local_status != HOST_STATUS_S && etr->cm.HS_term > overseer->log->HS_term) {
        if (DEBUG_LEVEL >= 4) {
            printf("Dist HS-term (%d) is greater than local (%d).\n", etr->cm.HS_term, overseer->log->HS_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        overseer->log->HS_term = etr->cm.HS_term;
        hl_update_status(overseer, etr->cm.status, etr->cm.host_id);
        if (local_status == HOST_STATUS_HS)
            stepdown_to_cs(overseer);
    }

    // If local is not S and local HS-term is greater than dist
    if (local_status != HOST_STATUS_S && etr->cm.HS_term < overseer->log->HS_term) {
        if (DEBUG_LEVEL >= 4) {
            printf("Local HS-term (%d) is greater than dist (%d).\n", overseer->log->HS_term, etr->cm.HS_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (etr->cm.status == HOST_STATUS_HS) {
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_INDICATE_HS,
                                        etr->cm.ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to reply with an INDICATE HS.\n");
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }
        if (cm_sendto(overseer,
                      sender_addr,
                      socklen,
                      MSG_TYPE_INDICATE_HS) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to reply with an INDICATE HS.\n");
    }

    // Else local is S or HS-terms are equal

    // Variable to hold if log is up-to-date and the new entry can be added. If not, it will be cached.
    int utd_check = 1;

    // If dist next index is greater than local and !(message type is new entry and dist next index equals local + 1)
    // and there isn't any log repair or replay ongoing already
    if (etr->cm.next_index > overseer->log->next_index &&
        !(etr->cm.next_index == overseer->log->next_index + 1 &&
          (etr->cm.type == MSG_TYPE_ETR_NEW || etr->cm.type == MSG_TYPE_ETR_NEW_AND_ACK)) &&
        (log_repair_ongoing(overseer) == 0 || log_replay_ongoing(overseer) == 0)) {
        if (DEBUG_LEVEL >= 4) {
            printf("Dist next index (%ld) is greater than local (%ld), and no log fix is ongoing.\n",
                   etr->cm.next_index,
                   overseer->log->next_index);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        // If latest entry in the log is in a committed state or if the latest pending entry in the log has the same
        // term as dist
        if ((overseer->log->next_index > 1 &&
             overseer->log->entries[overseer->log->next_index - 1].state == ENTRY_STATE_COMMITTED) ||
            overseer->log->entries[overseer->log->next_index - 1].term == etr->cm.P_term)
            log_replay(overseer, &etr->cm);
        else log_repair(overseer, &etr->cm);

        utd_check = 0;
    }

    // If local next index is greater than dist
    if (etr->cm.next_index < overseer->log->next_index) {
        if (DEBUG_LEVEL >= 4) {
            printf("Local next index (%ld) is greater than dist (%ld).\n",
                   overseer->log->next_index,
                   etr->cm.next_index);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (log_repair_ongoing(overseer) == 1)
            log_repair_override(overseer, &etr->cm);
        else log_repair(overseer, &etr->cm);
        utd_check = 0;
    }

    // Else next indexes are equal or message type is new entry and dist next index equals local + 1
    uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);

    switch (etr->cm.type) {
        case MSG_TYPE_ETR_NEW_AND_ACK:
            if (local_status != HOST_STATUS_S) {
                debug_log(0,
                          stderr,
                          "Fatal error: A master node should not receive a NEW ENTRY AND ACK message.\n");
                exit(EXIT_FAILURE);
            }
                    __attribute__ ((fallthrough));
        case MSG_TYPE_ETR_NEW:
            if (utd_check == 0)
                log_add_entry(overseer, etr, ENTRY_STATE_CACHED);
            else log_add_entry(overseer, etr, etr->state);
            if (local_status == HOST_STATUS_S & utd_check == 1) {
                if (server_send_first_prop(overseer, etr->cm.ack_reference) != EXIT_SUCCESS) {
                    debug_log(0, stderr, "Failed to Ack back New Entry and send Proposition.\n");
                    return EXIT_FAILURE;
                }
            } else if (cm_sendto_with_rt_init(overseer,
                                              sender_addr,
                                              socklen,
                                              MSG_TYPE_ACK_ENTRY,
                                              CM_DEFAULT_RT_ATTEMPTS,
                                              0,
                                              etr->cm.ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to Ack back New Entry.\n");
                return EXIT_FAILURE;
            }
            break;

        case MSG_TYPE_ETR_COMMIT:
            if (utd_check == 0) {
                debug_log(2, stdout, "Log check was not validated, caching received entry.\n");
                log_add_entry(overseer, etr, ENTRY_STATE_CACHED);
            } else log_add_entry(overseer, etr, ENTRY_STATE_COMMITTED);
            if (cm_sendto_with_rt_init(overseer,
                                       sender_addr,
                                       socklen,
                                       MSG_TYPE_ACK_COMMIT,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       etr->cm.ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to Ack back Commit Order.\n");
                return EXIT_FAILURE;
            }
            break;

        case MSG_TYPE_ETR_LOGFIX:
            // If the local version of the received entry has a different term or if the entry is marked invalid
            if (overseer->log->entries[etr->index].state == ENTRY_STATE_INVALID ||
                overseer->log->entries[etr->index].term != etr->term) {
                debug_log(2, stdout, "Local version of the entry is invalid.\n");
                // If the local version of the previous entry than the one received has a different term
                if (overseer->log->next_index > 1 && overseer->log->entries[etr->index - 1].term != etr->prev_term) {
                    debug_log(2,
                              stdout,
                              "Local version of the previous entry is also invalid, caching the received one.\n");
                    log_add_entry(overseer, etr, ENTRY_STATE_CACHED);
                    overseer->log->entries[etr->index - 1].state = ENTRY_STATE_INVALID;
                    errno = 0;
                    log_repair(overseer, &etr->cm);
                    if (errno != ENONE && cm_sendto_with_rt_init(overseer,
                                                                 overseer->hl->hosts[p_id].addr,
                                                                 overseer->hl->hosts[p_id].socklen,
                                                                 MSG_TYPE_ACK_ENTRY,
                                                                 CM_DEFAULT_RT_ATTEMPTS,
                                                                 0,
                                                                 etr->cm.ack_reference) != EXIT_SUCCESS) {
                        debug_log(0,
                                  stderr,
                                  "Failed to Ack back Entry to P after receiving Logfix from HS.\n");
                    }
                    return EXIT_SUCCESS;
                }

                // If local next index is 1 and received entry does not have index 1, or if the local version of the
                // previous entry than the one received has the same term
                if (overseer->log->next_index <= 1 && etr->index != 1)
                    log_add_entry(overseer, etr, ENTRY_STATE_CACHED);
                else log_add_entry(overseer, etr, etr->state);
            }

            // If the program reaches this point, the local log is (now) fixed up until the last added entry
            debug_log(3, stdout, "Log coherency verified, proceeding to replay.\n");

            uint64_t cached_set = 0;

            // While local next index is smaller than dist
            while (overseer->log->next_index < etr->cm.next_index) {
                log_entry_s *e = &overseer->log->entries[overseer->log->next_index];
                // If entry in the next index is cached and has the same term as dist host (not the received entry)
                if (e->state == ENTRY_STATE_CACHED && e->term == etr->cm.P_term) {
                    // Set it in the right state
                    if (etr->cm.commit_index >= overseer->log->next_index)
                        log_commit_upto(overseer, overseer->log->next_index);
                    else if (etr->cm.next_index >= overseer->log->next_index) {
                        e->state = ENTRY_STATE_PENDING;
                        overseer->log->next_index++;
                    }
                    cached_set++;
                } else {
                    if (DEBUG_LEVEL >= 3 && cached_set > 0) {
                        printf("%ld cached entries set to their correct state.\n",
                               cached_set);
                        if (INSTANT_FFLUSH) fflush(stdout);
                    }
                    log_replay(overseer, &etr->cm);
                    if (etr->cm.status == HOST_STATUS_HS) {
                        if (cm_sendto_with_rt_init(overseer,
                                                   overseer->hl->hosts[p_id].addr,
                                                   overseer->hl->hosts[p_id].socklen,
                                                   MSG_TYPE_ACK_ENTRY,
                                                   CM_DEFAULT_RT_ATTEMPTS,
                                                   0,
                                                   etr->cm.ack_reference) != EXIT_SUCCESS) {
                            debug_log(0,
                                      stderr,
                                      "Failed to Ack back Entry to P after receiving Logfix from HS.\n");
                        }
                    }
                    return EXIT_SUCCESS;
                }
            }
            if (DEBUG_LEVEL >= 3 && cached_set > 0) {
                printf("%ld cached entries set to their correct state.\n",
                       cached_set);
                if (INSTANT_FFLUSH) fflush(stdout);
            }

            // At this point the log is in a valid and up-to-date state as far as the current logfix is concerned
            log_fix_end(overseer);
            debug_log(3, stdout, "Acknowledging end of fix:\n");
            if (etr->cm.status == HOST_STATUS_HS) {
                // Ack entry to P
                if (cm_sendto_with_rt_init(overseer,
                                           overseer->hl->hosts[p_id].addr,
                                           overseer->hl->hosts[p_id].socklen,
                                           MSG_TYPE_ACK_ENTRY,
                                           CM_DEFAULT_RT_ATTEMPTS,
                                           0,
                                           etr->cm.ack_reference) != EXIT_SUCCESS) {
                    debug_log(0,
                              stderr,
                              "Failed to Ack back Entry to P after receiving Logfix from HS.\n");
                }
                // Generic Ack back to HS
                if (cm_sendto_with_ack_back(overseer,
                                            sender_addr,
                                            socklen,
                                            MSG_TYPE_GENERIC_ACK,
                                            etr->cm.ack_reference) != EXIT_SUCCESS) {
                    debug_log(0,
                              stderr,
                              "Failed to Generic Ack back to HS after receiving Logfix from HS.\n");
                }
            } else if (cm_sendto_with_rt_init(overseer,
                                              sender_addr,
                                              socklen,
                                              MSG_TYPE_ACK_ENTRY,
                                              CM_DEFAULT_RT_ATTEMPTS,
                                              0,
                                              etr->cm.ack_reference) != EXIT_SUCCESS) {
                debug_log(0,
                          stderr,
                          "Failed to Ack back Entry after receiving Logfix.\n");
            }
            break;

        default:
            if (local_status == HOST_STATUS_S) {
                debug_log(0,
                          stderr,
                          "Fatal error: Invalid message type received by server node.\n");
                exit(EXIT_FAILURE);
            }
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_INDICATE_P,
                                        etr->cm.ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to indicate P back after receiving ETR of invalid type.\n");
                return EXIT_FAILURE;
            }
    }

    return EXIT_SUCCESS;
}

int server_send_first_prop(overseer_s *overseer, uint32_t ack_back) {
    ops_queue_s *opq = ops_queue_pop(overseer->mfs);
    if (opq == NULL)
        return EXIT_SUCCESS;

    debug_log(2, stdout, "Sending the first Op in the queue.\n");

    entry_transmission_s *netr = etr_new(overseer,
                                         MSG_TYPE_ETR_PROPOSITION,
                                         opq->op,
                                         overseer->log->next_index,
                                         overseer->log->P_term,
                                         ENTRY_STATE_PROPOSAL,
                                         ack_back);
    ops_queue_element_free(opq);

    errno = 0;
    uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
    if (p_id == EXIT_FAILURE && errno == ENONE)
        return EXIT_FAILURE;

    if (etr_sendto_with_rt_init(overseer,
                                overseer->hl->hosts[p_id].addr,
                                overseer->hl->hosts[p_id].socklen,
                                netr,
                                PROPOSITION_RETRANSMISSION_DEFAULT_ATTEMPTS) != EXIT_SUCCESS) {
        // Cleanup and abort in case of failure, including subsequent elements
        // Note: no risk of dangling pointer since queue was empty
        // FIXME Improvement: only reset queue if all attempts to send fail
        fprintf(stderr,
                "Failed to transmit proposition.\n"
                "Resetting the queue: %d elements freed.\n",
                ops_queue_free_all(overseer, overseer->mfs->queue));
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}