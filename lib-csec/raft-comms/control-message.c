//
// Created by zheraan on 26/04/23.
//

#include "control-message.h"

control_message_s *cm_new(const overseer_s *overseer, enum message_type type, uint32_t ack_back) {
    control_message_s *ncm = malloc(sizeof(control_message_s));

    if (ncm == NULL) {
        perror("malloc new control message struct");
        if (INSTANT_FFLUSH) fflush(stderr);
        return (NULL);
    }

    // Use P/HS values in case of INDICATE P/HS message
    if (type == MSG_TYPE_INDICATE_P) {
        errno = 0;
        uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
        if (p_id == EXIT_FAILURE && errno == ENONE) {
            debug_log(0, stderr, "Requesting INDICATE P message but no host has P status\n");
            free(ncm);
            return (NULL);
        }
        ncm->host_id = p_id;
    } else if (type == MSG_TYPE_INDICATE_HS) {
        errno = 0;
        uint32_t hs_id = hl_whois(overseer->hl, HOST_STATUS_HS);
        if (hs_id == EXIT_FAILURE && errno == ENONE) {
            debug_log(0, stderr, "Requesting INDICATE HS message but no host has HS status\n");
            free(ncm);
            return (NULL);
        }
        ncm->host_id = hs_id;
    } else {
        ncm->host_id = overseer->hl->localhost_id;
    }
    ncm->status = overseer->hl->hosts[ncm->host_id].status;
    ncm->type = type;
    ncm->ack_reference = 0;
    ncm->ack_back = ack_back;
    ncm->next_index = overseer->log->next_index;
    // If CM is vote or voting bid
    if (type == MSG_TYPE_HS_VOTE || type == MSG_TYPE_HS_VOTING_BID)
        ncm->commit_index = overseer->es->bid_number; // Use commit index field as bid number
    else ncm->commit_index = overseer->log->commit_index;
    ncm->P_term = overseer->log->P_term;
    ncm->HS_term = overseer->log->HS_term;

    return ncm;
}

void cm_print(const control_message_s *hb, FILE *stream) {
    fprintf(stream,
            "   > host_id:       %d\n"
            "   > status:        %d\n"
            "   > type:          %d\n"
            "   > ack_reference: %d\n"
            "   > ack_back:      %d\n"
            "   > next_index:    %ld\n"
            "   > commit_index:  %ld\n"
            "   > P_term:        %d\n"
            "   > HS_term:       %d\n",
            hb->host_id,
            hb->status,
            hb->type,
            hb->ack_reference,
            hb->ack_back,
            hb->next_index,
            hb->commit_index,
            hb->P_term,
            hb->HS_term);
    if (INSTANT_FFLUSH) fflush(stream);
    return;
}

void cm_print_type(const control_message_s *cm, FILE *stream) {
    // Responses depending on the type of control message
    switch (cm->type) {
        case MSG_TYPE_HB_DEFAULT:
            debug_log(0, stream, "-> is HB DEFAULT\n");
            break;

        case MSG_TYPE_GENERIC_ACK:
            debug_log(0, stream, "-> is GENERIC ACK\n");
            break;

        case MSG_TYPE_P_TAKEOVER:
            debug_log(0, stream, "-> is P TAKEOVER\n");
            break;

        case MSG_TYPE_HS_TAKEOVER:
            debug_log(0, stream, "-> is HS TAKEOVER\n");
            break;

        case MSG_TYPE_LOG_REPAIR:
            debug_log(0, stream, "-> is LOG REPAIR\n");
            break;

        case MSG_TYPE_LOG_REPLAY:
            debug_log(0, stream, "-> is LOG REPLAY\n");
            break;

        case MSG_TYPE_ACK_ENTRY:
            debug_log(0, stream, "-> is ACK ENTRY\n");
            break;

        case MSG_TYPE_ACK_COMMIT:
            debug_log(0, stream, "-> is ACK COMMIT\n");
            break;

        case MSG_TYPE_INDICATE_P:
            debug_log(0, stream, "-> is INDICATE P\n");
            break;

        case MSG_TYPE_INDICATE_HS:
            debug_log(0, stream, "-> is INDICATE HS\n");
            break;

        case MSG_TYPE_NETWORK_PROBE:
            debug_log(0, stream, "-> is NETWORK PROBE\n");
            break;

        case MSG_TYPE_ETR_COMMIT:
            debug_log(0, stream, "-> is ETR COMMIT\n");
            break;

        case MSG_TYPE_ETR_NEW:
            debug_log(0, stream, "-> is ETR NEW\n");
            break;

        case MSG_TYPE_ETR_PROPOSITION:
            debug_log(0, stream, "-> is ETR PROPOSITION\n");
            break;

        case MSG_TYPE_ETR_LOGFIX:
            debug_log(0, stream, "-> is ETR LOGFIX\n");
            break;

        case MSG_TYPE_ETR_NEW_AND_ACK:
            debug_log(0, stream, "-> is ETR NEW AND ACK\n");
            break;

        default:
            fprintf(stderr, "Invalid control message type %d\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
    }

    return;
}

int cm_sendto(overseer_s *overseer,
              struct sockaddr_in6 sockaddr,
              socklen_t socklen,
              enum message_type type) {
    return cm_sendto_with_rt_init(overseer, sockaddr, socklen, type, 0, 0, 0);
}

int cm_sendto_with_ack_back(overseer_s *overseer,
                            struct sockaddr_in6 sockaddr,
                            socklen_t socklen,
                            enum message_type type,
                            uint32_t ack_back) {
    return cm_sendto_with_rt_init(overseer, sockaddr, socklen, type, 0, 0, ack_back);
}

int cm_sendto_with_rt_init(overseer_s *overseer,
                           struct sockaddr_in6 sockaddr,
                           socklen_t socklen,
                           enum message_type type,
                           uint8_t rt_attempts,
                           uint32_t ack_reference,
                           uint32_t ack_back) {

    if (DEBUG_LEVEL >= 3) {
        if (rt_attempts > 0)
            printf("Sending CM of type %d with %d retransmission rt_attempts ... \n", type, rt_attempts);
        else
            printf("Sending CM of type %d ... \n", type);
        if (INSTANT_FFLUSH) fflush(stdout);
    }

    control_message_s *cm = cm_new(overseer, type, ack_back);
    if (cm == NULL) {
        fprintf(stderr, "Failed to create message of type %d", type);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    if (rt_attempts == 0 && ack_reference == 0)
        cm->ack_reference = 0;
    else if (ack_reference == 0) {
        uint32_t rv = rtc_add_new(overseer, rt_attempts, sockaddr, socklen, type, NULL, ack_back);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            return EXIT_FAILURE;
        }
        cm->ack_reference = rv;
        debug_log(4, stdout, "CM RT init OK\n");
    } else cm->ack_reference = ack_reference;

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf("Sending to %s the following CM:\n", buf);
        cm_print(cm, stdout);
    }

    do {
        errno = 0;
        if (sendto(overseer->socket_cm,
                   cm,
                   sizeof(control_message_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1) {
            perror("CM sendto");
            if (INSTANT_FFLUSH) fflush(stderr);
            if (errno != EAGAIN) {
                free(cm);
                return EXIT_FAILURE;
            }
        }
    } while (errno == EAGAIN);

    free(cm);

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int cm_reception_init(overseer_s *overseer) {
    debug_log(4, stdout, "- Initializing next control message reception event ... ");
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->socket_cm,
                                              EV_READ,
                                              cm_receive_cb,
                                              (void *) overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Fatal error: failed to create the next CM reception event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Message reception has low priority
    event_priority_set(reception_event, 1);

    if (overseer->cm_reception_event != NULL) // Freeing the past event if any
        event_free(overseer->cm_reception_event);
    overseer->cm_reception_event = reception_event;

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next CM reception event.\n");
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void cm_receive_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout, "Start of CM reception callback ----------------------------------------------------\n");

    control_message_s cm;
    struct sockaddr_in6 sender_addr;
    socklen_t socklen = sizeof(sender_addr);

    do {
        errno = 0;
        if (recvfrom(fd,
                     &cm,
                     sizeof(control_message_s),
                     0,
                     (struct sockaddr *) &sender_addr,
                     &socklen) == -1) {
            perror("CM recvfrom");
            if (INSTANT_FFLUSH) fflush(stderr);
            if (errno != EAGAIN) {
                debug_log(1, stdout, "Failure receiving CM.\n");
                cm_reception_init((overseer_s *) arg);
                debug_log(4, stdout,
                          "End of CM reception callback ------------------------------------------------------\n\n");
                return; // Failure
            }
        }
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 2) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender_addr.sin6_addr), buf, 256);
        printf("Received from %s (aka. %s) a CM:\n", buf, ((overseer_s *) arg)->hl->hosts[cm.host_id].name);
        if (DEBUG_LEVEL >= 3)
            cm_print(&cm, stdout);
    }

    // If the incoming message is acknowledging a previously sent message, remove its retransmission cache
    if (cm.ack_back != 0) {
        if (DEBUG_LEVEL >= 4) {
            printf("-> Ack back value is non-zero (%d), removing corresponding RT cache entry ... ", cm.ack_back);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (rtc_remove_by_id((overseer_s *) arg, cm.ack_back, FLAG_DEFAULT) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");
        else
            debug_log(4,
                      stdout,
                      "Failure. The entry may have been removed earlier due to timeout.\n");
        // TODO Improvement: Log an error in case time was lower than timeout somehow or figure a way
    }

    if (DEBUG_LEVEL >= 2)
        cm_print_type(&cm, stdout);

    // Take any actions that need be
    if (cm_actions((overseer_s *) arg, sender_addr, socklen, &cm) != EXIT_SUCCESS)
        debug_log(1, stderr, "CM action failure.\n");

    // Init next event
    cm_reception_init((overseer_s *) arg); // Fatal error in case of failure anyway, so no need for a check

    debug_log(4, stdout, "End of CM reception callback ------------------------------------------------------\n\n");
    return;
}

void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("CM retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((retransmission_cache_s *) arg)->cur_attempts + 1);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    int success = 1;

    // Send message to P
    if (cm_sendto_with_rt_init(((retransmission_cache_s *) arg)->overseer,
                               ((retransmission_cache_s *) arg)->addr,
                               ((retransmission_cache_s *) arg)->socklen,
                               ((retransmission_cache_s *) arg)->type,
                               0,
                               ((retransmission_cache_s *) arg)->id,
                               ((retransmission_cache_s *) arg)->ack_back)) {
        fprintf(stderr, "Failed retransmitting the control message\n");
        success = 0;
    }

    // Increase attempts number
    ((retransmission_cache_s *) arg)->cur_attempts += 1;

    // If attempts max reached, remove cache entry
    if (((retransmission_cache_s *) arg)->cur_attempts >= ((retransmission_cache_s *) arg)->max_attempts) {
        rtc_remove_by_id(((retransmission_cache_s *) arg)->overseer, ((retransmission_cache_s *) arg)->id,
                         FLAG_DEFAULT);
    } else { // Otherwise add retransmission event
        // Add the event in the loop
        struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
        if (errno == EUNKNOWN_TIMEOUT_TYPE ||
            event_add(((retransmission_cache_s *) arg)->ev, &ops_timeout) != 0) {
            fprintf(stderr, "Failed to add the CM retransmission event\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    return;
}

int cm_broadcast(overseer_s *overseer, enum message_type type, uint8_t rt_attempts, uint8_t flags) {
    host_s *target;
    uint32_t nb_hosts = overseer->hl->nb_hosts;
    uint32_t nb_cm = 0;

    if (DEBUG_LEVEL >= 3) {
        printf("Broadcasting CM of type %d to master pool ... ", type);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    for (uint32_t i = 0; i < nb_hosts; ++i) {
        target = &(overseer->hl->hosts[i]);

        // TODO Extension Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip iterations based on flags or if it is the local host
        if (((flags & FLAG_SKIP_CS) == FLAG_SKIP_CS && target->status == HOST_STATUS_CS) ||
            ((flags & FLAG_SKIP_HS) == FLAG_SKIP_HS && target->status == HOST_STATUS_HS) ||
            ((flags & FLAG_SKIP_P) == FLAG_SKIP_P && target->status == HOST_STATUS_P) ||
            ((flags & FLAG_SKIP_S) == FLAG_SKIP_S && target->type == NODE_TYPE_S) ||
            target->locality == HOST_LOCALITY_LOCAL)
            continue;

        if (DEBUG_LEVEL >= 4) {
            printf("\n- CM target: %s\n", target->name);
            if (INSTANT_FFLUSH) fflush(stdout);
        }

        if (cm_sendto_with_rt_init(overseer,
                                   target->addr,
                                   target->socklen,
                                   type,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   0) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to send and RT init heartbeat\n");
        } else nb_cm++;
    }

    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d CMs sent).\n", nb_cm);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    return EXIT_SUCCESS;
}

int cm_actions(overseer_s *overseer,
               struct sockaddr_in6 sender_addr,
               socklen_t socklen,
               control_message_s *cm) {
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    // If local is P or HS and P-terms are equal and message is not Indicate P, Indicate HS, HS Vote or HS Voting Bid
    if ((local_status == HOST_STATUS_P || local_status == HOST_STATUS_HS) &&
        cm->P_term == overseer->log->P_term &&
        cm->type != MSG_TYPE_INDICATE_P &&
        cm->type != MSG_TYPE_INDICATE_HS &&
        cm->type != MSG_TYPE_HS_VOTE &&
        cm->type != MSG_TYPE_HS_VOTING_BID) {
        // Update local version of next index
        hl_host_index_change(overseer, cm->host_id, cm->next_index, cm->commit_index);
    }

    // If message has superior or equal P-term and comes from P and local is not already P
    if (cm->P_term >= overseer->log->P_term && cm->status == HOST_STATUS_P && local_status != HOST_STATUS_P)
        p_liveness_set_timeout(overseer); // Reset P liveness timer
    // If message has superior or equal HS-term and comes from HS and local is not already HS
    if (cm->HS_term >= overseer->log->HS_term && cm->status == HOST_STATUS_HS && local_status != HOST_STATUS_HS)
        election_set_timeout(overseer); // Reset HS election timer

    // If CM is a default HB, takeover message or network probe message
    if (cm->type == MSG_TYPE_HB_DEFAULT ||
        cm->type == MSG_TYPE_NETWORK_PROBE ||
        cm->type == MSG_TYPE_HS_TAKEOVER ||
        cm->type == MSG_TYPE_P_TAKEOVER) {
        // If local host is a master node
        if (local_status != HOST_STATUS_S)
            return hb_actions_as_master(overseer, sender_addr, socklen, cm);
        else  // If local host is a server node
            return hb_actions_as_server(overseer, sender_addr, socklen, cm);
    }

    // If CM is elections-related
    if (cm->type == MSG_TYPE_HS_VOTING_BID || cm->type == MSG_TYPE_HS_VOTE)
        return cm_election_actions(overseer, sender_addr, socklen, cm);

    // Else if CM is any other type

    if (local_status == HOST_STATUS_P || local_status == HOST_STATUS_HS) // If local host is P or HS
        return cm_other_actions_as_p_hs(overseer, sender_addr, socklen, cm);
    else // Else is S or CS
        return cm_other_actions_as_s_cs(overseer, sender_addr, socklen, cm);
}

int hb_actions_as_master(overseer_s *overseer,
                         struct sockaddr_in6 sender_addr,
                         socklen_t socklen,
                         control_message_s *cm) {
    // Save local status
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    if (cm->P_term > overseer->log->P_term) { // If dist P-term is greater
        debug_log(4, stdout, "Dist P-term is greater than local\n");
        int rv = hl_update_status(overseer, cm->status, cm->host_id);

        if (local_status != HOST_STATUS_CS) { // If local host is P or HS
            debug_log(4, stdout, "Stepping down to CS ... ");
            if (stepdown_to_cs(overseer) != EXIT_SUCCESS)
                rv = EXIT_FAILURE;
            else debug_log(4, stdout, "Done.\n");
        }

        // Acknowledge CM by HB back
        debug_log(4, stdout, "Answering with HB DEFAULT ... ");
        if (cm_sendto_with_rt_init(overseer,
                                   sender_addr,
                                   socklen,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   cm->ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to send and RT init an HB DEFAULT\n");
            rv = EXIT_FAILURE;
        } else debug_log(4, stdout, "Done.\n");

        debug_log(4, stdout, "Starting Log Repair ... ");
        if (log_repair(overseer, cm) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to start Log Repair\n");
            rv = EXIT_FAILURE;
        } else debug_log(4, stdout, "Done.\n");

        return rv;
    }

    if (cm->P_term < overseer->log->P_term) { // If local P_term is greater
        debug_log(4, stdout, "Local P-term is greater than dist\n");
        if (local_status == HOST_STATUS_P) {
            // Heartbeat back without ack
            debug_log(4, stdout, "Sending back a heartbeat ... ");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_HB_DEFAULT,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send and RT init a HB DEFAULT\n");
                return EXIT_FAILURE;
            } else debug_log(4, stdout, "Done.\n");
        } else { // Local status is HS or CS
            // Indicate P back without ack
            debug_log(4, stdout, "Sending back an Indicate P message ... ");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_INDICATE_P,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send an INDICATE P\n");
                return EXIT_FAILURE;
            } else debug_log(4, stdout, "Done.\n");
        }

        return EXIT_SUCCESS;
    }

    // Else if local and dist P-terms are equal

    if (cm->HS_term > overseer->log->HS_term) { // If dist HS-term is greater
        if (cm->status == HOST_STATUS_HS)
            hl_update_status(overseer, HOST_STATUS_HS, cm->host_id); // Auto steps down if necessary
        election_state_reset(overseer); // Reset election state
        overseer->log->HS_term = cm->HS_term;
    }

    if (cm->HS_term < overseer->log->HS_term) { // If local HS-term is greater
        if (local_status == HOST_STATUS_CS || local_status == HOST_STATUS_P) {
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_INDICATE_HS,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to reply with INDICATE HS");
                return EXIT_FAILURE;
            }
        } else {
            if (cm_sendto_with_rt_init(overseer,
                                       sender_addr,
                                       socklen,
                                       MSG_TYPE_HB_DEFAULT,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to reply and RT init with HB DEFAULT");
                return EXIT_FAILURE;
            }
        }
        return EXIT_SUCCESS;
    }

    // Else if local and dist HS-terms are equal
    debug_log(4, stdout, "Updating dist host status in the Hosts-list ... ");
    if (hl_update_status(overseer, cm->status, cm->host_id) == EXIT_SUCCESS)
        debug_log(4, stdout, "Done.\n");

    if (cm->next_index > overseer->log->next_index) { // If dist next_index is greater than local
        debug_log(4, stdout, "Dist next index is greater than local\n");

        if (local_status == HOST_STATUS_P) {
            debug_log(0,
                      stderr,
                      "Fatal error: P cannot have a smaller next index if its term is equal.\n");
            exit(EXIT_FAILURE);
        }

        if (local_status == HOST_STATUS_HS && cm->status == HOST_STATUS_CS) {
            debug_log(0,
                      stderr,
                      "Fatal error: HS cannot receive messages from CS if P-terms are equal.\n");
            exit(EXIT_FAILURE);
        }

        // If the log is being repaired or replayed
        if (log_replay_ongoing(overseer) == 1 || log_repair_ongoing(overseer) == 1) {
            // Ack back with ack
            debug_log(4, stdout, "Log is already being repaired or replayed, replying with GENERIC ACK ... ");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            else debug_log(4, stdout, "Done.\n");
        } else {
            // If the latest entry in the log committed or if the latest entry in the log has the same term as dist
            if (overseer->log->commit_index == overseer->log->next_index - 1) {
                debug_log(4, stdout, "Last log entry is committed, starting Log Replay.\n");
                return log_replay(overseer, cm);
            } else if (overseer->log->next_index != 1 &&
                       overseer->log->entries[overseer->log->next_index - 1].term == cm->P_term) {
                debug_log(4, stdout, "Last log entry has the same term as dist, starting Log Replay.\n");
                return log_replay(overseer, cm);
            } else {
                debug_log(4, stdout, "Potential log inconsistencies detected, starting Log Repair.\n");
                return log_repair(overseer, cm);
            }
        }
    }

    if (cm->next_index < overseer->log->next_index) { // If local next_index is greater than dist
        debug_log(4, stdout, "Local next index is greater than dist\n");
        if (local_status == HOST_STATUS_P) { // If local is P
            // Send Logfix corresponding to dist's next index
            debug_log(4, stdout, "Replying with Logfix.\n");
            return etr_reply_logfix(overseer, cm);
        }

        if (log_repair_ongoing(overseer) == 1) {
            debug_log(4, stdout, "Overriding outdated Log Repair process.\n");
            // TODO Improvement implement log repair override
        }

        log_invalidate_from(overseer->log, cm->next_index);
        debug_log(4, stdout, "Starting Log Repair.\n");
        return log_repair(overseer, cm);
    }

    // Else if local and dist next_indexes are equal

    if (cm->commit_index < overseer->log->commit_index) { // If local commit index is greater
        debug_log(4, stdout, "Local commit index is greater than dist.\n");
        if (local_status == HOST_STATUS_CS) {
            debug_log(0,
                      stderr,
                      "Fatal error: CS cannot have greater commit index if P-terms are equal.\n");
            exit(EXIT_FAILURE);
        }

        if (local_status == HOST_STATUS_P || (local_status == HOST_STATUS_HS && cm->status == HOST_STATUS_CS)) {
            // Send commit order with latest committed entry
            debug_log(4, stdout, "Sending heartbeat for target node to update its commit index.\n");
            return cm_sendto_with_rt_init(overseer,
                                          sender_addr,
                                          socklen,
                                          MSG_TYPE_HB_DEFAULT,
                                          CM_DEFAULT_RT_ATTEMPTS,
                                          0,
                                          cm->ack_reference);
        }

        // Else if local is HS and dist is P
        debug_log(0,
                  stderr,
                  "Fatal error: HS cannot have greater commit index than P if P-terms are equal.\n");
        exit(EXIT_FAILURE);
    }

    if (cm->commit_index > overseer->log->commit_index) { // If dist commit index is greater
        debug_log(4, stdout, "Dist commit index is greater than local.\n");
        if (local_status == HOST_STATUS_P) {
            debug_log(0,
                      stderr,
                      "Fatal error: P cannot have outdated commit index if P-terms are equal.\n");
            exit(EXIT_FAILURE);
        }

        // Else if local is HS or CS
        if (log_commit_upto(overseer, cm->commit_index) != EXIT_SUCCESS)
            debug_log(0,
                      stderr,
                      "Failure committing entries as result of HB check.\n");
    }

    // Else if commit indexes are equal

    // If dist is CS or if dist is HS and local is P, reply with HB Ack
    if (cm->status == HOST_STATUS_CS || (cm->status == HOST_STATUS_HS && local_status == HOST_STATUS_P)) {
        debug_log(4, stdout, "Everything is in order, replying with HB Ack.\n");
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_GENERIC_ACK,
                                    cm->ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to send and RT init an ACK HB\n");
    } else {
        debug_log(4, stdout, "Everything is in order, replying back with HB.\n");
        if (cm_sendto_with_rt_init(overseer,
                                   sender_addr,
                                   socklen,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   cm->ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to send and RT init an ACK HB\n");
    }

    return EXIT_SUCCESS;
}

int hb_actions_as_server(overseer_s *overseer,
                         struct sockaddr_in6 sender_addr,
                         socklen_t socklen,
                         control_message_s *cm) {

    if (cm->P_term < overseer->log->P_term) { // If local P_term is greater
        int rv = cm_sendto_with_ack_back(overseer,
                                         sender_addr,
                                         socklen,
                                         MSG_TYPE_INDICATE_P,
                                         cm->ack_reference);
        if (rv != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to Indicate P in response to HB with p_outdated P-Term.\n");

        // Outdated Master nodes return to CS status
        debug_log(4, stdout, "Updating dist host status in the Hosts-list ... ");
        if (hl_update_status(overseer, HOST_STATUS_CS, cm->host_id) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");

        return rv;
    }

    debug_log(4, stdout, "Updating dist host status in the Hosts-list ... ");
    if (hl_update_status(overseer, cm->status, cm->host_id) == EXIT_SUCCESS)
        debug_log(4, stdout, "Done.\n");

    // If dist P_term or Next index is greater
    int p_outdated = cm->P_term > overseer->log->P_term;
    if (p_outdated || cm->next_index > overseer->log->next_index) {
        if (p_outdated)
            debug_log(4, stdout, "Local P-term p_outdated.\n");
        else
            debug_log(4, stdout, "Local next index p_outdated.\n");

        if (log_repair_ongoing(overseer) || log_replay_ongoing(overseer)) {
            debug_log(4, stdout, "Log repair/replay already ongoing, replying with GENERIC ACK.\n");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
        }

        // If the log is empty or if its latest entry has the same term as dist
        if (overseer->log->next_index == 1 ||
            overseer->log->entries[overseer->log->next_index - 1].term == cm->P_term)
            return log_replay(overseer, cm);
        return log_repair(overseer, cm);
    }

    // Else if local and dist P-terms are equal

    if (cm->commit_index < overseer->log->commit_index) { // If local commit index is greater
        debug_log(0,
                  stderr,
                  "Fatal error: an S host cannot have greater commit index if P-terms are equal.\n");
        exit(EXIT_FAILURE);
    }

    if (cm->commit_index > overseer->log->commit_index) { // If dist commit index is greater
        debug_log(4, stdout, "Dist commit index is greater.\n");
        log_commit_upto(overseer, cm->commit_index);
    }

    // Else if local and dist commit index are equal
    debug_log(4, stdout, "Everything is in order, replying with GENERIC ACK.\n");
    if (cm_sendto_with_ack_back(overseer,
                                sender_addr,
                                socklen,
                                MSG_TYPE_GENERIC_ACK,
                                cm->ack_reference) != EXIT_SUCCESS)
        debug_log(0, stderr, "Failed to send a GENERIC ACK\n");

    return EXIT_SUCCESS;
}

int cm_election_actions(overseer_s *overseer,
                        struct sockaddr_in6 sender_addr,
                        socklen_t socklen,
                        control_message_s *cm) {
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    // If local is S, or if local is P and CM is a vote
    if (local_status == HOST_STATUS_S || (local_status == HOST_STATUS_P && cm->type == MSG_TYPE_HS_VOTE)) {
        fprintf(stderr,
                "Fatal error: a node with host status %d should not receive a CM of type %d.\n",
                local_status,
                cm->type);
        if (INSTANT_FFLUSH) fflush(stderr);
        exit(EXIT_FAILURE);
    }

    switch (cm->type) {
        case MSG_TYPE_HS_VOTE:
            if (cm->HS_term != overseer->log->HS_term || // If HS-terms don't match
                overseer->es->candidacy != CANDIDACY_HS || // If the local node is not a candidate
                cm->commit_index != overseer->es->bid_number) // If vote does not concern the right bid
                break; // Ignore vote

            overseer->es->vote_count++;
            // Ack back CM
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to ack back HS Vote.\n");

            // If majority is reached, step up as the new HS
            if (overseer->es->vote_count >= overseer->log->master_majority)
                return promote_to_hs(overseer);
            return EXIT_SUCCESS;

        case MSG_TYPE_HS_VOTING_BID:
            if (cm->HS_term < overseer->log->HS_term || // If dist HS-term is inferior to local
                cm->P_term < overseer->log->P_term || // If dist P-term is inferior to local
                cm->next_index < overseer->log->next_index || // If dist next index is inferior to local
                cm->commit_index < overseer->es->bid_number || // If dist bid number is inferior to local
                cm->commit_index <=
                overseer->es->last_voted_bid) // If local node has already voted for this bid or a higher one
                break; // Ignore bid

            overseer->log->HS_term = cm->HS_term; // Set the HS-term if dist was higher
            overseer->es->bid_number = cm->commit_index; // Set the local bid if dist was higher
            overseer->es->last_voted_bid = cm->commit_index; // Keep track of vote
            end_hs_candidacy_round(overseer); // Stop local candidacy

            // Reply with HS vote
            return cm_sendto_with_rt_init(overseer,
                                          sender_addr,
                                          socklen,
                                          MSG_TYPE_HS_VOTE,
                                          CM_DEFAULT_RT_ATTEMPTS,
                                          0,
                                          cm->ack_back);

        default:
            fprintf(stderr, "Fatal error: Invalid election CM type %d.\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
            exit(EXIT_FAILURE);
    }

    // send a generic ack back
    int rv = cm_sendto_with_ack_back(overseer,
                                     sender_addr,
                                     socklen,
                                     MSG_TYPE_GENERIC_ACK,
                                     cm->ack_reference);
    if (rv != EXIT_SUCCESS)
        fprintf(stderr, "Failed to ack back election CM of type %d.\n", cm->type);
    return rv;
}

int cm_other_actions_as_s_cs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm) {
    // Save local status
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    switch (cm->type) {
        case MSG_TYPE_GENERIC_ACK:
            debug_log(3, stdout, "-> CM is of type GENERIC ACK\n");
                    __attribute__ ((fallthrough));
        case MSG_TYPE_INDICATE_P:
            debug_log(3, stdout, "-> CM is of type INDICATE P\n");
            if (cm->P_term < overseer->log->P_term) { // If local P-term is greater
                debug_log(4, stdout, "Local P-term is greater.\n");
                if (cm_sendto(overseer,
                              sender_addr,
                              socklen,
                              MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    debug_log(0,
                              stderr,
                              "Failed to send CM of type Indicate P.\n");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }

            if (cm->P_term > overseer->log->P_term) // If dist P-term is greater
                debug_log(4, stdout, "Dist P-term is greater.\n");
            else // Else if P-terms are the same
                debug_log(4, stdout, "P-terms are equal.\n");
            break;

        case MSG_TYPE_INDICATE_HS:
            debug_log(3, stdout, "-> CM is of type INDICATE HS\n");
            if (local_status != HOST_STATUS_CS) {
                debug_log(0, stderr, "Fatal error: should not receive INDICATE HS as a S host\n");
                exit(EXIT_FAILURE);
            }

            if (cm->HS_term < overseer->log->HS_term) { // If local HS-term is greater
                debug_log(4, stdout, "Local HS-term is greater.\n");
                if (cm_sendto(overseer,
                              sender_addr,
                              socklen,
                              MSG_TYPE_INDICATE_HS) != EXIT_SUCCESS) {
                    debug_log(0,
                              stderr,
                              "Failed to send CM of type Indicate HS.\n");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }

            if (cm->HS_term > overseer->log->HS_term) // If dist HS-term is greater
                debug_log(4, stdout, "Dist HS-term is greater.\n");
            else // Else if HS-terms are the same
                debug_log(4, stdout, "HS-terms are equal.\n");
            break;

        default:
            fprintf(stderr, "Fatal error: Invalid CM type %d\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
            exit(EXIT_FAILURE);
    }

    return hl_update_status(overseer, cm->status, cm->host_id);
}

int cm_other_actions_as_p_hs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm) {
    // Save local status
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    switch (cm->type) {
        case MSG_TYPE_LOG_REPLAY:
            debug_log(3, stdout, "-> CM is of type LOG REPLAY\n");
                    __attribute__ ((fallthrough));
        case MSG_TYPE_LOG_REPAIR:
            debug_log(3, stdout, "-> CM is of type LOG REPAIR\n");
            debug_log(4, stdout, "Replying with Logfix ... ");
            if (etr_reply_logfix(overseer, cm) == EXIT_SUCCESS)
                debug_log(4, stdout, "Done.\n");
            break;

        case MSG_TYPE_GENERIC_ACK:
            debug_log(3, stdout, "-> CM is of type GENERIC ACK\n");
            // Do nothing
            break;

        case MSG_TYPE_ACK_COMMIT:
            debug_log(3, stdout, "-> CM is of type ACK COMMIT\n");
                    __attribute__ ((fallthrough));
        case MSG_TYPE_ACK_ENTRY:
            debug_log(3, stdout, "-> CM is of type ACK ENTRY\n");
            if (local_status == HOST_STATUS_HS) {
                errno = 0;
                uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
                if (!(p_id == EXIT_FAILURE && errno == ENONE) && cm_forward(overseer,
                                                                            overseer->hl->hosts[p_id].addr,
                                                                            overseer->hl->hosts[p_id].socklen,
                                                                            cm) != EXIT_SUCCESS)
                    debug_log(0, stderr, "Failure forwarding Ack to P.\n");
            }
            // We ack back to the host the id refers to instead of the sender because it may have been forwarded
            if (cm_sendto_with_ack_back(overseer,
                                        overseer->hl->hosts[cm->host_id].addr,
                                        overseer->hl->hosts[cm->host_id].socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                if (cm->type == MSG_TYPE_ACK_ENTRY)
                    debug_log(4, stderr, "Failure acknowledging CM of type ACK ENTRY.\n");
                else debug_log(4, stderr, "Failure acknowledging CM of type ACK COMMIT.\n");
            }
            break;

        case MSG_TYPE_INDICATE_P:
            debug_log(3, stdout, "-> CM is of type INDICATE P\n");
            if (overseer->log->P_term > cm->P_term) {
                if (local_status == HOST_STATUS_P) {
                    if (cm_sendto_with_rt_init(overseer,
                                               sender_addr,
                                               socklen,
                                               MSG_TYPE_HB_DEFAULT,
                                               CM_DEFAULT_RT_ATTEMPTS,
                                               0,
                                               cm->ack_reference) != EXIT_SUCCESS)
                        debug_log(0,
                                  stderr,
                                  "Failed to send CM of type HB DEFAULT.\n");
                } else {
                    if (cm_sendto_with_ack_back(overseer,
                                                sender_addr,
                                                socklen,
                                                MSG_TYPE_INDICATE_P,
                                                cm->ack_reference) != EXIT_SUCCESS)
                        debug_log(0,
                                  stderr,
                                  "Failed to send CM of type INDICATE P.\n");
                }
            } else if (overseer->log->P_term < cm->P_term) {
                if (local_status == HOST_STATUS_P)
                    stepdown_to_cs(overseer);
                if (overseer->log->next_index > 1 &&
                    overseer->log->entries[overseer->log->next_index - 1].term == cm->P_term)
                    log_replay(overseer, cm);
                else log_repair(overseer, cm);
            }
            break;

        case MSG_TYPE_INDICATE_HS:
            debug_log(3, stdout, "-> CM is of type INDICATE HS\n");
            if (overseer->log->HS_term > cm->HS_term) {
                if (local_status == HOST_STATUS_HS) {
                    if (cm_sendto_with_rt_init(overseer,
                                               sender_addr,
                                               socklen,
                                               MSG_TYPE_HB_DEFAULT,
                                               CM_DEFAULT_RT_ATTEMPTS,
                                               0,
                                               cm->ack_reference) != EXIT_SUCCESS)
                        debug_log(0,
                                  stderr,
                                  "Failed to send CM of type HB DEFAULT.\n");
                } else {
                    if (cm_sendto_with_ack_back(overseer,
                                                sender_addr,
                                                socklen,
                                                MSG_TYPE_INDICATE_HS,
                                                cm->ack_reference) != EXIT_SUCCESS)
                        debug_log(0,
                                  stderr,
                                  "Failed to send CM of type INDICATE HS.\n");
                }
            } else if (overseer->log->HS_term < cm->HS_term && local_status == HOST_STATUS_HS)
                stepdown_to_cs(overseer);
            break;

        default:
            fprintf(stderr, "Fatal error: Invalid CM type %d\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
            exit(EXIT_FAILURE);
    }

    // If local and dist are P and dist has outdated P-term, set it to CS in local HL
    if (local_status == HOST_STATUS_P && cm->status == HOST_STATUS_P && cm->P_term < overseer->log->P_term)
        return hl_update_status(overseer, HOST_STATUS_CS, cm->host_id);
    // If local and dist are HS and dist has outdated HS-term, set it to CS in local HL
    if (local_status == HOST_STATUS_HS && cm->status == HOST_STATUS_HS && cm->HS_term < overseer->log->HS_term)
        return hl_update_status(overseer, HOST_STATUS_CS, cm->host_id);
    // Otherwise update normally
    return hl_update_status(overseer, cm->status, cm->host_id);
}


int cm_forward(const overseer_s *overseer,
               struct sockaddr_in6 sockaddr,
               socklen_t socklen,
               const control_message_s *cm) {
    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf(" - Forwarding to %s the following CM:\n", buf);
        cm_print(cm, stdout);
    }

    do {
        errno = 0;
        if (sendto(overseer->socket_cm,
                   cm,
                   sizeof(control_message_s),
                   0,
                   (const struct sockaddr *) &sockaddr,
                   socklen) == -1) {
            perror("CM sendto");
            if (INSTANT_FFLUSH) fflush(stderr);
            if (errno != EAGAIN)
                return EXIT_FAILURE;
        }
    } while (errno == EAGAIN);

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}