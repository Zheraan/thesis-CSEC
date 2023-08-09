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
            fflush(stdout);
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

void cm_print(const control_message_s *cm, FILE *stream, int flags) {
    char status_string[32];
    host_status_string(status_string, cm->status);
    char type_string[32];
    cm_type_string(type_string, cm->type);
    if ((flags & FLAG_PRINT_SHORT) == FLAG_PRINT_SHORT)
        fprintf(stream, "[HID %d,  %s,  %s,  AckR %d,  AckB %d,  NIx %ld,  CIx %ld,  PTerm %d,  HSTerm %d]\n",
                cm->host_id,
                status_string,
                type_string,
                cm->ack_reference,
                cm->ack_back,
                cm->next_index,
                cm->commit_index,
                cm->P_term,
                cm->HS_term);
    else
        fprintf(stream,
                "   > host_id:       %d\n"
                "   > status:        %d (%s)\n"
                "   > type:          %d (%s)\n"
                "   > ack_reference: %d\n"
                "   > ack_back:      %d\n"
                "   > next_index:    %ld\n"
                "   > commit_index:  %ld\n"
                "   > P_term:        %d\n"
                "   > HS_term:       %d\n",
                cm->host_id,
                cm->status,
                status_string,
                cm->type,
                type_string,
                cm->ack_reference,
                cm->ack_back,
                cm->next_index,
                cm->commit_index,
                cm->P_term,
                cm->HS_term);
    if (INSTANT_FFLUSH) fflush(stream);
    return;
}

void cm_type_string(char *buf, enum message_type type) {
    switch (type) {
        case MSG_TYPE_HB_DEFAULT:
            sprintf(buf, "HB DEFAULT");
            break;

        case MSG_TYPE_P_TAKEOVER:
            sprintf(buf, "P TAKEOVER");
            break;

        case MSG_TYPE_HS_TAKEOVER:
            sprintf(buf, "HS TAKEOVER");
            break;

        case MSG_TYPE_NETWORK_PROBE:
            sprintf(buf, "NETWORK PROBE");
            break;

        case MSG_TYPE_LOG_REPAIR:
            sprintf(buf, "LOG REPAIR");
            break;

        case MSG_TYPE_LOG_REPLAY:
            sprintf(buf, "LOG REPLAY");
            break;

        case MSG_TYPE_GENERIC_ACK:
            sprintf(buf, "GENERIC ACK");
            break;

        case MSG_TYPE_ACK_ENTRY:
            sprintf(buf, "ACK ENTRY");
            break;

        case MSG_TYPE_ACK_COMMIT:
            sprintf(buf, "ACK COMMIT");
            break;

        case MSG_TYPE_INDICATE_P:
            sprintf(buf, "INDICATE P");
            break;

        case MSG_TYPE_INDICATE_HS:
            sprintf(buf, "INDICATE HS");
            break;

        case MSG_TYPE_HS_VOTING_BID:
            sprintf(buf, "HS VOTING BID");
            break;

        case MSG_TYPE_HS_VOTE:
            sprintf(buf, "HS VOTE");
            break;

        case MSG_TYPE_ETR_COMMIT:
            sprintf(buf, "ETR COMMIT");
            break;

        case MSG_TYPE_ETR_NEW:
            sprintf(buf, "ETR NEW");
            break;

        case MSG_TYPE_ETR_PROPOSITION:
            sprintf(buf, "ETR PROPOSITION");
            break;

        case MSG_TYPE_ETR_LOGFIX:
            sprintf(buf, "ETR LOGFIX");
            break;

        case MSG_TYPE_ETR_NEW_AND_ACK:
            sprintf(buf, "ETR NEW AND ACK");
            break;
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

    control_message_s *ncm = cm_new(overseer, type, ack_back);
    if (ncm == NULL) {
        fprintf(stderr, "Failed to create message of type %d", type);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    if (rt_attempts == 0 && ack_reference == 0)
        ncm->ack_reference = 0;
    else if (ack_reference == 0) {
        uint32_t rv = rtc_add_new(overseer, rt_attempts, sockaddr, socklen, type, NULL, ack_back);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            return EXIT_FAILURE;
        }
        ncm->ack_reference = rv;
    } else ncm->ack_reference = ack_reference;

    if (DEBUG_LEVEL >= 3) {
        char addr_buf[256];
        evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), addr_buf, 256);

        if (DEBUG_LEVEL >= 4) {
            if (rt_attempts > 0)
                printf("Sending to %s the following CM with %d retransmission attempts:\n", addr_buf, rt_attempts);
            else
                printf("Sending to %s the following CM:\n", addr_buf);
            cm_print(ncm, stdout, CSEC_FLAG_DEFAULT);
        } else if (DEBUG_LEVEL >= 2) {
            if (rt_attempts > 0)
                printf("Sending to %s a CM with %d RT attempts:\n- ", addr_buf, rt_attempts);
            else
                printf("Sending to %s a CM:\n- ", addr_buf);
            cm_print(ncm, stdout, FLAG_PRINT_SHORT);
        }
    }

    if (FUZZER_ENABLED) {
        fuzzer_entry_init(overseer, PACKET_TYPE_CM, (union packet) *ncm, sockaddr, socklen);
    } else {
        sockaddr.sin6_port = htons(PORT_CM);
        do {
            // If the fuzzer is disabled, send it normally
            errno = 0;
            if (sendto(overseer->socket_cm,
                       ncm,
                       sizeof(control_message_s),
                       0,
                       (const struct sockaddr *) &sockaddr,
                       socklen) == -1) {
                perror("CM sendto");
                if (INSTANT_FFLUSH) fflush(stderr);
                if (errno != EAGAIN) {
                    free(ncm);
                    return EXIT_FAILURE;
                }
            }
        } while (errno == EAGAIN);
    }

    free(ncm);

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int cm_reception_init(overseer_s *overseer) {
    debug_log(4, stdout, "Initializing next control message reception event ... ");
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
    debug_log(4, stdout,
              "Start of CM reception callback -----------------------------------------------------------------\n");

    overseer_s *overseer = (overseer_s *) arg;
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
                cm_reception_init(overseer);
                debug_log(4, stdout,
                          "End of CM reception callback -------------------------------------------------------------------\n\n");
                if (DEBUG_LEVEL == 3)
                    printf("\n");
                return; // Failure
            }
        }
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL == 2) printf("Received from %s a CM", overseer->hl->hosts[cm.host_id].name);
    else if (DEBUG_LEVEL >= 3) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender_addr.sin6_addr), buf, 256);
        printf("Received from %s (aka. %s) a CM", buf, overseer->hl->hosts[cm.host_id].name);
    }

    if (DEBUG_LEVEL == 1 || DEBUG_LEVEL == 2) {
        char type_string[32];
        cm_type_string(type_string, cm.type);
        printf(" of type %s\n", type_string);
    } else if (DEBUG_LEVEL >= 4) {
        printf(":\n");
        cm_print(&cm, stdout, CSEC_FLAG_DEFAULT);
    } else if (DEBUG_LEVEL == 3) {
        printf(":\n- ");
        cm_print(&cm, stdout, FLAG_PRINT_SHORT);
    }

    // If the incoming message is acknowledging a previously sent message, remove its retransmission cache
    if (cm.ack_back != 0) {
        if (DEBUG_LEVEL >= 4) {
            printf("Ack back value is non-zero (%d), removing corresponding RT cache entry ... ", cm.ack_back);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (rtc_remove_by_id(overseer, cm.ack_back, CSEC_FLAG_DEFAULT) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");
        else
            debug_log(4,
                      stdout,
                      "Failure. The entry may have been removed earlier due to timeout.\n");
        // TODO Improvement: Log an error in case time was lower than timeout somehow or figure a way
    }

    // Take any actions that need be
    if (cm_actions(overseer, sender_addr, socklen, &cm) != EXIT_SUCCESS)
        debug_log(1, stderr, "CM action failure.\n");

    // Init next event
    cm_reception_init(overseer); // Fatal error in case of failure anyway, so no need for a check

    debug_log(4, stdout,
              "End of CM reception callback -------------------------------------------------------------------\n\n");

    if (DEBUG_LEVEL == 3)
        printf("\n");
    return;
}

void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout,
              "Start of CM retransmission callback ------------------------------------------------------------\n");

    retransmission_cache_s *rtc = arg;
    if (DEBUG_LEVEL >= 3) {
        printf("CM retransmission timed out, reattempting transmission (attempt %d of %d) ... ",
               rtc->cur_attempts + 1,
               rtc->max_attempts);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    int success = 1;

    // Send message to P
    if (cm_sendto_with_rt_init(rtc->overseer,
                               rtc->addr,
                               rtc->socklen,
                               rtc->type,
                               0,
                               rtc->id,
                               rtc->ack_back)) {
        fprintf(stderr, "Failed retransmitting the control message\n");
        success = 0;
    }

    // Increase attempts number
    rtc->cur_attempts += 1;

    // If attempts max reached, remove cache entry
    if (rtc->cur_attempts >= rtc->max_attempts) {
        rtc_remove_by_id(rtc->overseer, rtc->id, CSEC_FLAG_DEFAULT);
    } else { // Otherwise add retransmission event
        // Add the event in the loop
        struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
        if (errno == EUNKNOWN_TIMEOUT_TYPE ||
            event_add(rtc->ev, &ops_timeout) != 0) {
            fprintf(stderr, "Failed to add the CM retransmission event\n");
            if (INSTANT_FFLUSH) fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    debug_log(4, stdout,
              "End of CM retransmission callback --------------------------------------------------------------\n\n");

    if (DEBUG_LEVEL == 3)
        printf("\n");
    return;
}

int cm_broadcast(overseer_s *overseer, enum message_type type, uint8_t rt_attempts, uint8_t flags) {
    host_s *target;
    uint32_t nb_hosts = overseer->hl->nb_hosts;
    uint32_t nb_cm = 0;

    if (DEBUG_LEVEL >= 2) {
        char broadcast_targets_string[32] = "";
        if (!((flags & FLAG_SKIP_CS) == FLAG_SKIP_CS))
            strcat(broadcast_targets_string, " CS");
        if (!((flags & FLAG_SKIP_HS) == FLAG_SKIP_HS))
            strcat(broadcast_targets_string, " HS");
        if (!((flags & FLAG_SKIP_P) == FLAG_SKIP_P))
            strcat(broadcast_targets_string, " P");
        if (!((flags & FLAG_SKIP_S) == FLAG_SKIP_S))
            strcat(broadcast_targets_string, " S");

        char type_string[32];
        cm_type_string(type_string, type);
        printf("Broadcasting CM of type %d (%s) to%s nodes ... ", type, type_string, broadcast_targets_string);
        debug_log(3, stdout, "\n");
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    for (uint32_t i = 0; i < nb_hosts; ++i) {
        target = &(overseer->hl->hosts[i]);

        // TODO Extension Add conditional re-resolving of nodes that are of unknown or unreachable status

        // Skip iterations based on flags, if it's a monitor or if it is the local host
        if (((flags & FLAG_SKIP_CS) == FLAG_SKIP_CS && target->status == HOST_STATUS_CS) ||
            ((flags & FLAG_SKIP_HS) == FLAG_SKIP_HS && target->status == HOST_STATUS_HS) ||
            ((flags & FLAG_SKIP_P) == FLAG_SKIP_P && target->status == HOST_STATUS_P) ||
            ((flags & FLAG_SKIP_S) == FLAG_SKIP_S && target->type == NODE_TYPE_S) ||
            target->type == NODE_TYPE_CM ||
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

    if (DEBUG_LEVEL >= 2) {
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
        hl_replication_index_change(overseer, cm->host_id, cm->next_index, cm->commit_index);
    }

    // If message has superior or equal P-term and comes from P and local is not already P
    if (cm->P_term >= overseer->log->P_term && cm->status == HOST_STATUS_P && local_status != HOST_STATUS_P) {
        debug_log(4, stdout, "Message is from P, resetting Liveness timeout.\n");
        p_liveness_set_timeout(overseer); // Reset P liveness timer
    }
    // If message has superior or equal HS-term and comes from HS and local is CS
    if (cm->status == HOST_STATUS_HS && cm->HS_term >= overseer->log->HS_term && local_status == HOST_STATUS_CS) {
        debug_log(4, stdout, "Message is from HS, resetting election timeout.\n");
        election_set_timeout(overseer); // Reset HS election timer
    }

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
        // If P-term is only outdated because of a takeover but all other parameters indicate coherence
        if (cm->type == MSG_TYPE_P_TAKEOVER &&
            log_repair_ongoing(overseer) == false &&
            log_replay_ongoing(overseer) == false &&
            overseer->log->P_term == cm->P_term - 1 &&
            overseer->log->next_index == cm->next_index) {
            debug_log(3, stdout, "Acknowledging clean P takeover, incrementing P-term.\n");
            overseer->log->P_term++;
            int rv = hl_update_status(overseer, cm->status, cm->host_id);
            // Just ack back, in order to avoid unnecessary logfixes
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
                return EXIT_FAILURE;
            }
            return rv;
        }

        if (DEBUG_LEVEL >= 3) {
            printf("Dist P-term (%d) is greater than local (%d).\n",
                   cm->P_term,
                   overseer->log->P_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        int rv = hl_update_status(overseer, cm->status, cm->host_id);

        if (local_status != HOST_STATUS_CS) { // If local host is P or HS
            if (stepdown_to_cs(overseer) != EXIT_SUCCESS)
                rv = EXIT_FAILURE;
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
        if (DEBUG_LEVEL >= 3) {
            printf("Local P-term (%d) greater than dist's (%d).\n",
                   overseer->log->P_term,
                   cm->P_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (local_status == HOST_STATUS_P) {
            // Heartbeat back without ack
            debug_log(4, stdout, "Sending back a HB DEFAULT ... ");
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
            debug_log(4, stdout, "Sending back an INDICATE P ... ");
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
        if (DEBUG_LEVEL >= 3) {
            printf("Dist HS-term (%d) is greater than local (%d).\n",
                   cm->HS_term,
                   overseer->log->HS_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (cm->status == HOST_STATUS_HS) {
            hl_update_status(overseer, HOST_STATUS_HS, cm->host_id); // Auto steps down if necessary
        }
        election_state_reset(overseer); // Reset election state
        overseer->log->HS_term = cm->HS_term;
    }

    if (cm->HS_term < overseer->log->HS_term) { // If local HS-term is greater
        if (DEBUG_LEVEL >= 3) {
            printf("Local HS-term (%d) is greater than dist (%d).\n",
                   overseer->log->HS_term,
                   cm->HS_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
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
    hl_update_status(overseer, cm->status, cm->host_id);

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

        // If the log is being repaired or replayed in another conversation
        if ((log_replay_ongoing(overseer) == true || log_repair_ongoing(overseer) == true) &&
            cm->ack_back != overseer->log->fix_conversation) {
            // Ack back with generic ack
            debug_log(4,
                      stdout,
                      "Another logfix conversation is already ongoing, replying with GENERIC ACK ... ");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            else debug_log(4, stdout, "Done.\n");
            // TODO Check later if this function doesn't need to not return there, and same for the server version
            return EXIT_SUCCESS;
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

        if (log_repair_ongoing(overseer) == true) {
            debug_log(4, stdout, "Overriding outdated Log Repair process.\n");
            log_repair_override(overseer, cm);
        }

        log_invalidate_from(overseer->log, cm->next_index);
        debug_log(4, stdout, "Starting Log Repair.\n");
        return log_repair(overseer, cm);
    }

    // Else if local and dist next_indexes are equal

    if (cm->commit_index < overseer->log->commit_index) { // If local commit index is greater
        debug_log(4, stdout, "Local commit index is greater than dist.\n");
        if (local_status == HOST_STATUS_CS) {
            if (cm->next_index < overseer->log->commit_index + 1) {
                debug_log(0,
                          stderr,
                          "Fatal error: CS cannot have greater commit index if P-terms are equal and dist next "
                          "index is inferior to local commit index + 1.\n");
                exit(EXIT_FAILURE);
            }
            debug_log(2,
                      stdout,
                      "Warning: local commit index is higher than dist's, however dist's next index indicates "
                      "recovery is possible.\n");
            if (cm_sendto_with_rt_init(overseer,
                                       sender_addr,
                                       socklen,
                                       MSG_TYPE_HB_DEFAULT,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
                return EXIT_FAILURE;
            }
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
        if (cm->next_index < overseer->log->commit_index + 1) {
            debug_log(0,
                      stderr,
                      "Fatal error: HS cannot have greater commit index than P if P-terms are equal and dist next "
                      "index is inferior to local commit index + 1.\n");
            exit(EXIT_FAILURE);
        }

        debug_log(2,
                  stdout,
                  "Warning: local commit index is higher than dist's, however dist's next index indicates "
                  "recovery is possible.\n");
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_GENERIC_ACK,
                                    cm->ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            return EXIT_FAILURE;
        }
    }

    if (cm->commit_index > overseer->log->commit_index) { // If dist commit index is greater
        if (DEBUG_LEVEL >= 3) {
            printf("Dist commit index (%ld) is greater than local (%ld).\n",
                   cm->commit_index,
                   overseer->log->commit_index);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        if (local_status == HOST_STATUS_P) {
            if (cm->commit_index + 1 > overseer->log->next_index) {
                debug_log(0,
                          stderr,
                          "Fatal error: P cannot have outdated commit index if P-terms are equal and local next "
                          "index is inferior to dist commit index + 1.\n");
                exit(EXIT_FAILURE);
            }
            debug_log(2,
                      stdout,
                      "Warning: dist commit index is higher than local, however local next index indicates "
                      "recovery is possible.\n");
            if (cm_sendto_with_rt_init(overseer,
                                       sender_addr,
                                       socklen,
                                       MSG_TYPE_HB_DEFAULT,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
                return EXIT_FAILURE;
            }
        }

        // Else if local is HS or CS
        if (log_commit_upto(overseer, cm->commit_index) != EXIT_SUCCESS)
            debug_log(0,
                      stderr,
                      "Failure committing entries as result of HB check.\n");
    }

    // Else if commit indexes are equal

    // If dist is CS or if dist is HS and local is P, reply with Generic Ack
    if (cm->status == HOST_STATUS_CS || (cm->status == HOST_STATUS_HS && local_status == HOST_STATUS_P)) {
        debug_log(4, stdout, "Everything is in order, replying with GENERIC ACK.\n");
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_GENERIC_ACK,
                                    cm->ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to send a GENERIC ACK.\n");
    } else {
        debug_log(4, stdout, "Everything is in order, replying back with HB DEFAULT.\n");
        if (cm_sendto_with_rt_init(overseer,
                                   sender_addr,
                                   socklen,
                                   MSG_TYPE_HB_DEFAULT,
                                   CM_DEFAULT_RT_ATTEMPTS,
                                   0,
                                   cm->ack_reference) != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to send and RT init an HB DEFAULT.\n");
    }

    return EXIT_SUCCESS;
}

int hb_actions_as_server(overseer_s *overseer,
                         struct sockaddr_in6 sender_addr,
                         socklen_t socklen,
                         control_message_s *cm) {

    if (cm->P_term < overseer->log->P_term) { // If local P_term is greater
        if (DEBUG_LEVEL >= 3) {
            printf("Local P-term (%d) is greater than dist (%d).\n",
                   overseer->log->P_term,
                   cm->P_term);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        int rv = cm_sendto_with_ack_back(overseer,
                                         sender_addr,
                                         socklen,
                                         MSG_TYPE_INDICATE_P,
                                         cm->ack_reference);
        if (rv != EXIT_SUCCESS)
            debug_log(0, stderr, "Failed to Indicate P in response to HB with p_outdated P-Term.\n");

        // Outdated Master nodes return to CS status
        hl_update_status(overseer, HOST_STATUS_CS, cm->host_id);

        return rv;
    }

    hl_update_status(overseer, cm->status, cm->host_id);

    // If dist P_term or Next index is greater
    int p_outdated = cm->P_term > overseer->log->P_term;
    if (p_outdated || cm->next_index > overseer->log->next_index) {
        // If P-term is only outdated because of a takeover but all other parameters indicate coherence
        if (cm->type == MSG_TYPE_P_TAKEOVER &&
            log_repair_ongoing(overseer) == false &&
            log_replay_ongoing(overseer) == false &&
            overseer->log->P_term == cm->P_term - 1 &&
            overseer->log->next_index == cm->next_index) {
            // Just ack back, in order to avoid unnecessary logfixes
            debug_log(3, stdout, "Acknowledging clean P takeover, incrementing P-term.\n");
            overseer->log->P_term++;
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        if (p_outdated) {
            if (DEBUG_LEVEL >= 2) {
                printf("Local P-term (%d) is outdated (dist is %d).\n",
                       overseer->log->P_term,
                       cm->P_term);
                if (INSTANT_FFLUSH) fflush(stdout);
            }
        } else {
            if (DEBUG_LEVEL >= 2) {
                printf("Local next index (%ld) is outdated (dist is %ld).\n",
                       overseer->log->next_index,
                       cm->next_index);
                if (INSTANT_FFLUSH) fflush(stdout);
            }
        }

        // Check if the log is already being repaired in another conversation and if so, just ack back
        if ((log_repair_ongoing(overseer) == true || log_replay_ongoing(overseer) == true) &&
            cm->ack_back != overseer->log->fix_conversation) {
            debug_log(4,
                      stdout,
                      "Another logfix conversation is already ongoing, replying with GENERIC ACK.\n");
            if (cm_sendto_with_ack_back(overseer,
                                        sender_addr,
                                        socklen,
                                        MSG_TYPE_GENERIC_ACK,
                                        cm->ack_reference) != EXIT_SUCCESS)
                debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            return EXIT_SUCCESS;
        }

        // If the log is empty or if its latest entry has the same term as dist
        if (overseer->log->next_index == 1 ||
            overseer->log->entries[overseer->log->next_index - 1].term == cm->P_term)
            return log_replay(overseer, cm);
        return log_repair(overseer, cm);
    }

    // Else if local and dist P-terms are equal

    if (cm->commit_index < overseer->log->commit_index && // If local commit index is greater and
        cm->next_index < overseer->log->commit_index + 1) { // dist next index is smaller than local commit index +1
        debug_log(0,
                  stderr,
                  "Fatal error: an S node cannot have greater commit index if P-terms are equal and dist next "
                  "index is inferior to local commit index + 1.\n");
        exit(EXIT_FAILURE);
    }

    if (cm->commit_index < overseer->log->commit_index && // If local commit index is greater and
        cm->next_index >= overseer->log->commit_index + 1) {// dist next index greater or equal to local commit index +1
        debug_log(2,
                  stdout,
                  "Warning: local commit index is higher than dist's, however dist's next index indicates "
                  "recovery is possible.\n");
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_GENERIC_ACK,
                                    cm->ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            return EXIT_FAILURE;
        }
    }

    if (cm->commit_index > overseer->log->commit_index) { // If dist commit index is greater
        if (DEBUG_LEVEL >= 3) {
            printf("Dist commit index (%ld) is greater than local (%ld).\n",
                   cm->commit_index,
                   overseer->log->commit_index);
            if (INSTANT_FFLUSH) fflush(stdout);
        }
        log_commit_upto(overseer, cm->commit_index);
    }

    // Else if local and dist commit index are equal

    // If the Ops-queue isn't empty, send first prop
    if (overseer->mfs->queue != NULL) {
        debug_log(4, stdout, "Everything is in order, but Ops-queue isn't empty. ");
        if (server_send_first_prop(overseer, cm->ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            return EXIT_FAILURE;
        }
    } else {
        debug_log(4, stdout, "Everything is in order, replying with GENERIC ACK.\n");
        if (cm_sendto_with_ack_back(overseer,
                                    sender_addr,
                                    socklen,
                                    MSG_TYPE_GENERIC_ACK,
                                    cm->ack_reference) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failed to send a GENERIC ACK\n");
            return EXIT_FAILURE;
        }
    }

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
                cm->commit_index != overseer->es->bid_number) { // If vote does not concern the right bid
                debug_log(3, stdout, "Invalid vote ignored.\n");
                break; // Ignore vote
            }

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
                cm->commit_index < overseer->es->bid_number) { // If dist bid number is inferior to local
                debug_log(3, stdout, "Invalid vote ignored.\n");
                break; // Ignore bid
            }
            // If local node has already voted for this bid or a higher one
            if (cm->commit_index <= overseer->es->last_voted_bid) {
                debug_log(3,
                          stdout,
                          "Bid ignored due to the local host already having given its vote this round or higher.\n");
                break; // Ignore bid
            }

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
        fprintf(stderr,
                "Failed to ack back election CM of type %d.\n", cm->type);
    return
            rv;
}

int cm_other_actions_as_s_cs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm) {
    // Save local status
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    switch (cm->type) {
        case MSG_TYPE_GENERIC_ACK:
                    __attribute__ ((fallthrough));
        case MSG_TYPE_INDICATE_P:
            if (cm->P_term < overseer->log->P_term) { // If local P-term is greater
                if (DEBUG_LEVEL >= 3) {
                    printf("Local P-term (%d) is greater than dist (%d).\n",
                           overseer->log->P_term,
                           cm->P_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
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

            if (cm->P_term > overseer->log->P_term) { // If dist P-term is greater
                if (DEBUG_LEVEL >= 3) {
                    printf("Dist P-term (%d) is greater than local (%d).\n",
                           cm->P_term,
                           overseer->log->P_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
            } else // Else if P-terms are the same
                debug_log(4, stdout, "P-terms are equal.\n");
            break;

        case MSG_TYPE_INDICATE_HS:
            if (local_status != HOST_STATUS_CS) {
                debug_log(0, stderr, "Fatal error: should not receive INDICATE HS as a S host\n");
                exit(EXIT_FAILURE);
            }

            if (cm->HS_term < overseer->log->HS_term) { // If local HS-term is greater
                if (DEBUG_LEVEL >= 3) {
                    printf("Local HS-term (%d) is greater than dist (%d).\n",
                           overseer->log->HS_term,
                           cm->HS_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
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

            if (cm->HS_term > overseer->log->HS_term) { // If dist HS-term is greater
                if (DEBUG_LEVEL >= 3) {
                    printf("Dist HS-term (%d) is greater than local (%d).\n",
                           cm->HS_term,
                           overseer->log->HS_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
            } else // Else if HS-terms are the same
                debug_log(4, stdout, "HS-terms are equal.\n");
            break;

        default:
            fprintf(stderr, "Fatal error: Invalid CM type %d\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
            exit(EXIT_FAILURE);
    }

    return hl_update_status(overseer, cm->status, cm->host_id);;
}

int cm_other_actions_as_p_hs(overseer_s *overseer,
                             struct sockaddr_in6 sender_addr,
                             socklen_t socklen,
                             control_message_s *cm) {
    // Save local status
    enum host_status local_status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    switch (cm->type) {
        case MSG_TYPE_LOG_REPLAY:
                    __attribute__ ((fallthrough));
        case MSG_TYPE_LOG_REPAIR:
            debug_log(4, stdout, "Replying with Logfix ... ");
            if (etr_reply_logfix(overseer, cm) == EXIT_SUCCESS)
                debug_log(4, stdout, "Done.\n");
            break;

        case MSG_TYPE_GENERIC_ACK:
            // Do nothing
            break;

        case MSG_TYPE_ACK_COMMIT:
                    __attribute__ ((fallthrough));
        case MSG_TYPE_ACK_ENTRY:
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
            if (overseer->log->P_term > cm->P_term) {
                if (DEBUG_LEVEL >= 3) {
                    printf("Local P-term (%d) is greater than dist (%d).\n",
                           overseer->log->P_term,
                           cm->P_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
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
                if (DEBUG_LEVEL >= 3) {
                    printf("Dist P-term (%d) is greater than local (%d).\n",
                           cm->P_term,
                           overseer->log->P_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
                if (local_status == HOST_STATUS_P)
                    stepdown_to_cs(overseer);
                if (overseer->log->next_index > 1 &&
                    overseer->log->entries[overseer->log->next_index - 1].term == cm->P_term)
                    log_replay(overseer, cm);
                else log_repair(overseer, cm);
            }
            break;

        case MSG_TYPE_INDICATE_HS:
            if (overseer->log->HS_term > cm->HS_term) {
                if (DEBUG_LEVEL >= 3) {
                    printf("Local HS-term (%d) is greater than dist (%d).\n",
                           overseer->log->HS_term,
                           cm->HS_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
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
            } else if (overseer->log->HS_term < cm->HS_term && local_status == HOST_STATUS_HS) {
                if (DEBUG_LEVEL >= 3) {
                    printf("Dist HS-term (%d) is greater than local (%d).\n",
                           cm->HS_term,
                           overseer->log->HS_term);
                    if (INSTANT_FFLUSH) fflush(stdout);
                }
                stepdown_to_cs(overseer);
            }
            break;

        default:
            fprintf(stderr, "Fatal error: Invalid CM type %d\n", cm->type);
            if (INSTANT_FFLUSH) fflush(stderr);
            exit(EXIT_FAILURE);
    }

    // If local and dist are P and dist has outdated P-term, set it to CS in local HL
    if (local_status == HOST_STATUS_P && cm->status == HOST_STATUS_P && cm->P_term < overseer->log->P_term) {
        return hl_update_status(overseer, HOST_STATUS_CS, cm->host_id);
    }
    // If local and dist are HS and dist has outdated HS-term, set it to CS in local HL
    if (local_status == HOST_STATUS_HS && cm->status == HOST_STATUS_HS && cm->HS_term < overseer->log->HS_term) {
        return hl_update_status(overseer, HOST_STATUS_CS, cm->host_id);
    }
    // Otherwise update normally
    return hl_update_status(overseer, cm->status, cm->host_id);;
}


int cm_forward(overseer_s *overseer,
               struct sockaddr_in6 sockaddr,
               socklen_t socklen,
               const control_message_s *cm) {
    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 4) {
        printf(" - Forwarding to %s the following CM:\n", buf);
        cm_print(cm, stdout, CSEC_FLAG_DEFAULT);
    } else if (DEBUG_LEVEL == 3) {
        printf(" - Forwarding to %s the following CM:\n", buf);
        cm_print(cm, stdout, FLAG_PRINT_SHORT);
    }

    if (FUZZER_ENABLED) {
        fuzzer_entry_init(overseer, PACKET_TYPE_CM, (union packet) *cm, sockaddr, socklen);
    } else {
        sockaddr.sin6_port = htons(PORT_CM);
        // If the fuzzer is disabled, send it normally
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
    }

    debug_log(3, stdout, "Done.\n");
    return EXIT_SUCCESS;
}