//
// Created by zheraan on 26/04/23.
//

#include "control-message.h"

control_message_s *cm_new(const overseer_s *overseer, enum message_type type, uint32_t ack_back) {
    control_message_s *ncm = malloc(sizeof(control_message_s));

    if (ncm == NULL) {
        perror("malloc new control message struct");
        fflush(stderr);
        return (NULL);
    }

    if (type == MSG_TYPE_INDICATE_P) {
        uint32_t p_id = whois_p(overseer->hl);
        if (p_id == 1 && errno == ENO_P) {
            fprintf(stderr, "Requesting INDICATE P message but no host has P status\n");
            fflush(stderr);
            free(ncm);
            return (NULL);
        }
        ncm->host_id = p_id;
    } else {
        ncm->host_id = overseer->hl->localhost_id;
    }
    ncm->status = overseer->hl->hosts[ncm->host_id].status;
    ncm->type = type;
    ncm->ack_reference = 0;
    ncm->ack_back = ack_back;
    ncm->next_index = overseer->log->next_index;
    ncm->match_index = overseer->log->match_index;
    ncm->commit_index = overseer->log->commit_index;
    ncm->term = overseer->log->P_term;

    return ncm;
}

void cm_print(const control_message_s *hb, FILE *stream) {
    fprintf(stream,
            "host_id:       %d\n"
            "status:        %d\n"
            "type:          %d\n"
            "ack_reference: %d\n"
            "ack_back:      %d\n"
            "next_index:    %ld\n"
            "commit_index:  %ld\n"
            "match_index:   %ld\n"
            "term:          %d\n",
            hb->host_id,
            hb->status,
            hb->type,
            hb->ack_reference,
            hb->ack_back,
            hb->next_index,
            hb->commit_index,
            hb->match_index,
            hb->term);
    fflush(stream);
    return;
}

int cm_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type) {
    return cm_sendto_with_rt_init(overseer, sockaddr, socklen, type, 0, 0, 0);
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
        fflush(stdout);
    }

    control_message_s *cm = cm_new(overseer, type, ack_back);
    if (cm == NULL) {
        fprintf(stderr, "Failed to create message of type %d", type);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    if (rt_attempts == 0 && ack_reference == 0)
        cm->ack_reference = 0;
    else if (ack_reference == 0) {
        uint32_t rv = rt_cache_add_new(overseer, rt_attempts, sockaddr, socklen, type, NULL, ack_back);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        cm->ack_reference = rv;
        debug_log(4, stdout, " - CM RT init OK\n");
    } else cm->ack_reference = ack_reference;

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf(" - Sending to %s the following CM:\n", buf);
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
            fflush(stderr);
            free(cm);
            if (errno != EAGAIN)
                return EXIT_FAILURE;
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
        fprintf(stderr, "Fatal error: failed to create the next CM reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Message reception has low priority
    event_priority_set(reception_event, 1);

    if (overseer->cm_reception_event != NULL) // Freeing the past event if any
        event_free(overseer->cm_reception_event);
    overseer->cm_reception_event = reception_event;

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next CM reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void cm_receive_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout, "Start of CM reception callback ----------------------------------------------------\n");

    control_message_s cm;
    struct sockaddr_in6 sender;
    socklen_t sender_len = sizeof(sender);

    do {
        errno = 0;
        if (recvfrom(fd,
                     &cm,
                     sizeof(control_message_s),
                     0,
                     (struct sockaddr *) &sender,
                     &sender_len) == -1) {
            perror("CM recvfrom");
            fflush(stderr);
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
        evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
        printf("Received from %s a CM:\n", buf);
        if (DEBUG_LEVEL >= 3)
            cm_print(&cm, stdout);
    }

    // If the incoming message is acknowledging a previously sent message, remove its retransmission cache
    if (cm.ack_back != 0) {
        debug_log(4,
                  stdout,
                  "-> Ack back value is non-zero, removing corresponding RT cache entry ... ");
        if (rt_cache_remove_by_id((overseer_s *) arg, cm.ack_back) == EXIT_SUCCESS)
            debug_log(4, stdout, "Done.\n");
        else debug_log(4, stderr, "Failure.\n");
    }

    enum cm_check_rv crv = cm_check_metadata((overseer_s *) arg, &cm);
    cm_check_action((overseer_s *) arg, crv, sender, sender_len, &cm); // TODO Failure handling

    // Responses depending on the type of control message
    switch (cm.type) {
        case MSG_TYPE_HB_DEFAULT:
            debug_log(2, stdout, "-> is HB DEFAULT\nAcking back: ");
            // Default heartbeat means replying with cm ack
            if (cm_sendto_with_rt_init(((overseer_s *) arg),
                                       sender,
                                       sender_len,
                                       MSG_TYPE_ACK_HB,
                                       0, // No need for RT
                                       0,
                                       cm.ack_reference) != EXIT_SUCCESS) {
                fprintf(stderr, "Failed to Ack heartbeat\n");
                fflush(stderr);
                return;
            }
            break;

        case MSG_TYPE_ACK_HB:
            debug_log(2, stdout, "-> is HB ACK\n");
            // TODO Apply any effects of ack (reset timeout for next ack...)
            break;

        case MSG_TYPE_P_TAKEOVER:
            debug_log(2, stdout, "-> is P TAKEOVER\n");
            // TODO Check term and apply any effects of P takeover if valid
            break;

        case MSG_TYPE_HS_TAKEOVER:
            debug_log(2, stdout, "-> is HS TAKEOVER\n");
            // TODO Effects of HS TAKEOVER reception
            break;

        case MSG_TYPE_LOG_REPAIR:
            debug_log(2, stdout, "-> is LOG REPAIR\n");
            // TODO Effects of LOG REPAIR reception
            break;

        case MSG_TYPE_LOG_REPLAY:
            debug_log(2, stdout, "-> is LOG REPLAY\n");
            // TODO Effects of LOG REPLAY reception
            break;

        case MSG_TYPE_ACK_ENTRY:
            debug_log(2, stdout, "-> is ACK ENTRY\n");
            // TODO Effects of ACK ENTRY reception
            break;

        case MSG_TYPE_ACK_COMMIT:
            debug_log(2, stdout, "-> is ACK COMMIT\n");
            // TODO Effects of ACK COMMIT reception
            break;

        case MSG_TYPE_INDICATE_P:
            debug_log(2, stdout, "-> is INDICATE P\n");
            // TODO Effects of INDICATE P reception
            break;

        case MSG_TYPE_INDICATE_HS:
            debug_log(2, stdout, "-> is INDICATE HS\n");
            // TODO implement Indicate HS and HS term
            break;

        default:
            fprintf(stderr, "Invalid control message type %d\n", cm.type);
            fflush(stderr);
    }

    cm_reception_init((overseer_s *) arg); // Fatal error in case of failure anyway, so no need for a check

    debug_log(4, stdout, "End of CM reception callback ------------------------------------------------------\n\n");
    return;
}

enum cm_check_rv cm_check_metadata(overseer_s *overseer, const control_message_s *hb) {
    // If local P-term is outdated
    if (hb->term > overseer->log->P_term) {
        if (DEBUG_LEVEL >= 1) {
            printf("Local term (%d) is outdated compared to received value (%d), updating local value.\n",
                   overseer->log->P_term,
                   hb->term);
            fflush(stdout);
        }
        // Adjust local P-term
        overseer->log->P_term = hb->term;

        // Indicate P messages have a different host ID value, therefore they are not considered here
        if (hb->type != MSG_TYPE_INDICATE_P) {
            if (DEBUG_LEVEL >= 1 && hb->status != overseer->hl->hosts[hb->host_id].status) {
                printf("Dist host status in the local hosts-list (%d) is outdated compared to received value "
                       "(%d), updating local value.\n",
                       overseer->hl->hosts[hb->host_id].status,
                       hb->status);
                fflush(stdout);
            }
            // If sender is either P or HS, adjust the identity of HS or P in the hosts list (resetting current P or HS)
            if (hb->status == HOST_STATUS_P || hb->status == HOST_STATUS_HS) {
                if (hl_change_master(overseer->hl, hb->status, hb->host_id) != EXIT_SUCCESS) {
                    fprintf(stderr, "CM check metadata error: failed to adjust master status\n");
                    return CHECK_RV_FAILURE;
                }
            }
                // Else set the status of the sender in the hosts list
            else overseer->hl->hosts[hb->host_id].status = hb->status;

            // And ask for repair in case log entries are outdated (cannot rely on indexes only if term was wrong)
            return CHECK_RV_NEED_REPAIR;
        }
        // If message is Indicate P, it is handled in the receiver function
    }

        // If dist P-term is outdated
    else if (hb->term < overseer->log->P_term) {
        if (DEBUG_LEVEL >= 1)
            printf("Dist term (%d) is outdated compared to local value (%d). ",
                   overseer->log->P_term,
                   hb->term);
        if (overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P) {
            debug_log(1, stdout, "Indicate P message required.\n");
            return CHECK_RV_NEED_INDICATE_P;
        } else {
            debug_log(1, stdout, "Transmission of the latest log entry required.\n");
            return CHECK_RV_NEED_ETR_LATEST;
        }
    }

    // Rest of function: P-terms are equal
    // TODO Implement HS-term checks too

    // If local commit index is outdated and local is not P (and term was up-to-date)
    if (hb->commit_index > overseer->log->commit_index &&
        overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P) {
        if (DEBUG_LEVEL >= 1) {
            printf("Local commit index (%ld) is outdated compared to received value (%ld), updating local value.\n",
                   overseer->log->commit_index,
                   hb->commit_index);
            fflush(stdout);
        }
        // TODO Commit up to commit index and update the index
    }
        // Else if dist commit is outdated do nothing and set the status of the sender in the hosts list
    else {
        if (DEBUG_LEVEL >= 1 && hb->status != overseer->hl->hosts[hb->host_id].status) {
            printf("Dist host status in the local hosts-list (%d) is outdated compared to received value "
                   "(%d), updating local value (P-terms were equal).\n",
                   overseer->hl->hosts[hb->host_id].status,
                   hb->status);
            fflush(stdout);
        }
        overseer->hl->hosts[hb->host_id].status = hb->status; // TODO figure a way to make a security check on this
    }

    // In case of Log Repair messages, indexes will have different values than their host's, so don't check for them
    // here to not send additional useless messages. Successive Log Repair indexes go backward in order, whilst
    // Log Repair go forward
    if (hb->type != MSG_TYPE_LOG_REPAIR) {
        if (hb->next_index > overseer->log->next_index) { // If local next index is outdated
            // Ask P for the next missing entry
            return CHECK_RV_NEED_REPLAY;
        } else if (hb->next_index < overseer->log->next_index) { // If dist next index is outdated
            // If local is P
            if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P) {
                // Send value entry corresponding to sender's next index
                return CHECK_RV_NEED_ETR_NEXT;
            }
            // Else if local is not P ignore message // TODO optimize with sending entry if you have it
        }
    }

    return CHECK_RV_CLEAN;
}

int cm_check_action(overseer_s *overseer,
                    enum cm_check_rv check_rv,
                    struct sockaddr_in6 addr,
                    socklen_t socklen,
                    control_message_s *cm) {
    debug_log(4, stdout, "Beginning of check action -----------------\n");
    int success = 1;

    switch (check_rv) {
        case CHECK_RV_CLEAN:
            debug_log(4, stdout, "CM metadata is valid.\n");
            break;

        case CHECK_RV_FAILURE:
            // Do nothing, failure is handled above
            debug_log(0, stderr, "CM metadata check failed.\n");
            success = 0;
            break;

        case CHECK_RV_NEED_REPAIR:
            debug_log(2, stdout, "CM P-term outdated, requiring Log Repair.\n");
            if (!is_p_available(overseer->hl))
                debug_log(0, stderr, "Local node requires Log Repair, but no P is available.\n");
            else if (cm_sendto_with_rt_init(overseer,
                                            overseer->hl->hosts[whois_p(overseer->hl)].addr,
                                            overseer->hl->hosts[whois_p(overseer->hl)].socklen,
                                            MSG_TYPE_LOG_REPAIR,
                                            CM_DEFAULT_RT_ATTEMPTS,
                                            0,
                                            0) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send and RT init CM of type Log Repair as result of check action\n");
                fflush(stderr);
                success = 0;
            }
            break;

        case CHECK_RV_NEED_REPLAY:
            if (cm_sendto_with_rt_init(overseer,
                                       addr,
                                       socklen,
                                       MSG_TYPE_LOG_REPLAY,
                                       CM_DEFAULT_RT_ATTEMPTS,
                                       0,
                                       0) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send and RT init CM of type Log Replay as result of check action\n");
                fflush(stderr);
                success = 0;
            }
            break;

        case CHECK_RV_NEED_INDICATE_P:
            if (cm_sendto(overseer,
                          addr,
                          socklen,
                          MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send CM of type Indicate P as result of check action\n");
                fflush(stderr);
                success = 0;
            }
            break;

        case CHECK_RV_NEED_ETR_LATEST:
            // Status check voluntarily redundant with CM check
            if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P) {
                debug_log(4, stdout, "- Sender needs latest entry, transmitting ... ");

                // Create new ETR based on latest entry in the log
                entry_transmission_s *netr = etr_new_from_local_entry(overseer,
                                                                      MSG_TYPE_ETR_LOGFIX,
                                                                      overseer->log->next_index - 1,
                                                                      0);
                if (netr == NULL) {
                    fprintf(stderr,
                            "Failed to create ETR for replying with entry corresponding to next index\n");
                    success = 0;
                    break;
                }

                // Send ETR
                if (etr_sendto_with_rt_init(overseer,
                                            netr,
                                            addr,
                                            socklen,
                                            MSG_TYPE_ETR_LOGFIX,
                                            ETR_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send and RT init ETR of type ETR Logfix as result of check action");
                    fflush(stderr);
                    success = 0;
                }
                debug_log(4, stdout, "Done.\n");
            } else { // Should not be necessary because of CM check but well who knows
                debug_log(4, stdout, "- Sender needs latest entry but local is not P\n");
                if (cm_sendto(overseer, addr, socklen, MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send CM of type Indicate P as result of check action\n");
                    fflush(stderr);
                    success = 0;
                }
            }
            break;

        case CHECK_RV_NEED_ETR_NEXT:
            // Status check voluntarily redundant with CM check
            if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P) {
                if (DEBUG_LEVEL >= 4) {
                    printf("- Sender needs entry in next index (%ld), transmitting ... ", cm->next_index);
                    fflush(stdout);
                }

                entry_transmission_s *netr = etr_new_from_local_entry(overseer,
                                                                      MSG_TYPE_ETR_LOGFIX,
                                                                      cm->next_index,
                                                                      0);
                if (netr == NULL) {
                    fprintf(stderr,
                            "Failed to create ETR for replying with entry corresponding to next index\n");
                    success = 0;
                    break;
                }

                // Send ETR
                if (etr_sendto_with_rt_init(overseer,
                                            netr,
                                            addr,
                                            socklen,
                                            MSG_TYPE_ETR_LOGFIX,
                                            ETR_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send and RT init ETR of type ETR Logfix as result of check action");
                    fflush(stderr);
                    success = 0;
                }
                debug_log(4, stdout, "Done.\n");
            } else { // Should not be necessary because of CM check but well who knows
                debug_log(4, stdout, "- Sender needs entry corresponding to next index but local is not P\n");
                if (cm_sendto(overseer, addr, socklen, MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send CM of type Indicate P as result of check action\n");
                    fflush(stderr);
                    success = 0;
                }
            }
            break;

        default:
            fprintf(stderr, "Unimplemented check check_rv %d\n", check_rv);
            fflush(stderr);
            success = 0;
            break;
    }
    debug_log(4, stdout, "End of check action -----------------------\n");
    if (success)
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("CM retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((retransmission_cache_s *) arg)->cur_attempts + 1);
        fflush(stdout);
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
        rt_cache_remove_by_id(((retransmission_cache_s *) arg)->overseer, ((retransmission_cache_s *) arg)->id);
    } else { // Otherwise add retransmission event
        // Add the event in the loop
        struct timeval ops_timeout = timeout_gen(TIMEOUT_TYPE_ACK);
        if (errno == EUNKNOWN_TIMEOUT_TYPE ||
            event_add(((retransmission_cache_s *) arg)->ev, &ops_timeout) != 0) {
            fprintf(stderr, "Failed to add the CM retransmission event\n");
            fflush(stderr);
            success = 0;
        }
    }

    if (success == 1)
        debug_log(3, stdout, "Done.\n");
    return;
}