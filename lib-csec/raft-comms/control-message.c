//
// Created by zheraan on 26/04/23.
//

#include "control-message.h"

control_message_s *cm_new(const overseer_s *overseer, enum message_type type) {
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
            "next_index:    %ld\n"
            "commit_index:  %ld\n"
            "match_index:   %ld\n"
            "term:          %d\n",
            hb->host_id,
            hb->status,
            hb->type,
            hb->next_index,
            hb->commit_index,
            hb->match_index,
            hb->term);
    fflush(stream);
    return;
}

int cm_sendto(overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type) {
    return cm_sendto_with_rt_init(overseer, sockaddr, socklen, type, 0);
}

int cm_sendto_with_rt_init(overseer_s *overseer,
                           struct sockaddr_in6 sockaddr,
                           socklen_t socklen,
                           enum message_type type,
                           uint8_t rt_attempts) {

    if (DEBUG_LEVEL >= 3) {
        printf("Sending CM of type %d with %d retransmission rt_attempts ... \n", type, rt_attempts);
        fflush(stdout);
    }

    control_message_s *cm = cm_new(overseer, type);
    if (cm == NULL) {
        fprintf(stderr, "Failed to create message of type %d", type);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    if (rt_attempts == 0)
        cm->ack_number = 0;
    else {
        uint32_t rv = rt_cache_add_new(overseer, rt_attempts, sockaddr, socklen, type, NULL);
        if (rv == 0) {
            fprintf(stderr, "Failed creating retransmission cache\n");
            fflush(stderr);
            return EXIT_FAILURE;
        }
        cm->ack_number = rv;

        if (DEBUG_LEVEL >= 4) {
            printf(" - CM RT init OK\n");
            fflush(stdout);
        }
    }

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
            return EXIT_FAILURE;
        }
    } while (errno == EAGAIN);

    free(cm);

    if (DEBUG_LEVEL >= 3) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

int cm_reception_init(overseer_s *overseer) {
    if (DEBUG_LEVEL >= 4) {
        printf("- Initializing next control message reception event ... ");
        fflush(stdout);
    }
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

    if (DEBUG_LEVEL >= 4) {
        printf("Done.\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

void cm_receive_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 4) {
        printf("Start of CM reception callback ----------------------------------------------------\n");
        fflush(stdout);
    }

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
        }
    } while (errno == EAGAIN);

    if (DEBUG_LEVEL >= 2) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender.sin6_addr), buf, 256);
        printf("Received from %s a CM:\n", buf);
        if (DEBUG_LEVEL >= 3)
            cm_print(&cm, stdout);
    }

    enum cm_check_rv crv = cm_check_metadata((overseer_s *) arg, &cm);
    cm_check_action((overseer_s *) arg, crv, sender, sender_len, &cm); // TODO Failure handling

    // Responses depending on the type of control message
    switch (cm.type) {
        case MSG_TYPE_HB_DEFAULT:
            if (DEBUG_LEVEL >= 2)
                printf("-> is HB DEFAULT\n");
            // Default heartbeat means replying with cm ack
            if (cm_sendto(((overseer_s *) arg),
                          sender,
                          sender_len,
                          MSG_TYPE_ACK_HB) != EXIT_SUCCESS) {
                fprintf(stderr, "Failed to Ack heartbeat\n");
                fflush(stderr);
                return;
            }
            break;

        case MSG_TYPE_ACK_HB:
            if (DEBUG_LEVEL >= 2)
                printf("-> is HB ACK\n");
            // TODO Apply any effects of ack (reset timeout for next ack...)
            break;

        case MSG_TYPE_P_TAKEOVER:
            if (DEBUG_LEVEL >= 2)
                printf("-> is P TAKEOVER\n");
            // TODO Check term and apply any effects of P takeover if valid
            break;

        case MSG_TYPE_HS_TAKEOVER:
            if (DEBUG_LEVEL >= 2)
                printf("-> is HS TAKEOVER\n");
            // TODO Effects of HS TAKEOVER reception
            break;

        case MSG_TYPE_LOG_REPAIR:
            if (DEBUG_LEVEL >= 2)
                printf("-> is LOG REPAIR\n");
            // TODO Effects of LOG REPAIR reception
            break;

        case MSG_TYPE_LOG_REPLAY:
            if (DEBUG_LEVEL >= 2)
                printf("-> is LOG REPLAY\n");
            // TODO Effects of LOG REPLAY reception
            break;

        case MSG_TYPE_ACK_ENTRY:
            if (DEBUG_LEVEL >= 2)
                printf("-> is ACK ENTRY\n");
            // TODO Effects of ACK ENTRY reception
            break;

        case MSG_TYPE_ACK_COMMIT:
            if (DEBUG_LEVEL >= 2)
                printf("-> is ACK COMMIT\n");
            // TODO Effects of ACK COMMIT reception
            break;

        case MSG_TYPE_INDICATE_P:
            if (DEBUG_LEVEL >= 2)
                printf("-> is INDICATE P\n");
            // TODO Effects of INDICATE P reception
            break;

        case MSG_TYPE_INDICATE_HS:
            if (DEBUG_LEVEL >= 2)
                printf("-> is INDICATE HS\n");
            // TODO implement Indicate HS and HS term
            break;

        default:
            fprintf(stderr, "Invalid control message type %d\n", cm.type);
            fflush(stderr);
    }

    cm_reception_init((overseer_s *) arg); // Fatal error in case of failure anyway, so no need for a check

    if (DEBUG_LEVEL >= 4) {
        printf("End of CM reception callback ------------------------------------------------------\n");
        fflush(stdout);
    }
    return;
}

enum cm_check_rv cm_check_metadata(overseer_s *overseer, const control_message_s *hb) {
    // If local P-term is outdated
    if (hb->term > overseer->log->P_term) {
        // Adjust local P-term
        overseer->log->P_term = hb->term;

        // Indicate P messages have
        if (hb->type != MSG_TYPE_INDICATE_P) {
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
        if (overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P)
            return CHECK_RV_NEED_INDICATE_P;
        else return CHECK_RV_NEED_ETR_LATEST;
    }

    // Rest of function: P-terms are equal
    // TODO Implement HS-term checks too

    // If local commit index is outdated and local is not P (and term was up-to-date)
    if (hb->commit_index > overseer->log->commit_index &&
        overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P) {
        // TODO Commit up to commit index
    }
        // Else if dist is outdated do nothing and set the status of the sender in the hosts list
    else overseer->hl->hosts[hb->host_id].status = hb->status; // TODO figure a way to make a security check on this

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
                    enum cm_check_rv rv,
                    struct sockaddr_in6 addr,
                    socklen_t socklen,
                    control_message_s *cm) {
    if (DEBUG_LEVEL >= 4) {
        printf("Beginning of check action ---------------------------------------------------------\n");
        fflush(stdout);
    }

    switch (rv) {
        case CHECK_RV_CLEAN:
        case CHECK_RV_FAILURE:
            // Do nothing, failure is handled above
            break;

        case CHECK_RV_NEED_REPAIR:
            if (cm_sendto_with_rt_init(overseer,
                                       addr,
                                       socklen,
                                       MSG_TYPE_LOG_REPAIR,
                                       CM_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send and RT init CM of type Log Repair as result of check action\n");
                fflush(stderr);
                return EXIT_FAILURE;
            }
            break;

        case CHECK_RV_NEED_REPLAY:
            if (cm_sendto_with_rt_init(overseer,
                                       addr,
                                       socklen,
                                       MSG_TYPE_LOG_REPLAY,
                                       CM_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send and RT init CM of type Log Replay as result of check action\n");
                fflush(stderr);
                return EXIT_FAILURE;
            }
            break;

        case CHECK_RV_NEED_INDICATE_P:
            if (cm_sendto_with_rt_init(overseer,
                                       addr,
                                       socklen,
                                       MSG_TYPE_INDICATE_P,
                                       CM_DEFAULT_RT_ATTEMPTS) != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Failed to send and RT init CM of type Indicate P as result of check action\n");
                fflush(stderr);
                return EXIT_FAILURE;
            }
            break;

        case CHECK_RV_NEED_ETR_LATEST:
            // Status check voluntarily redundant with CM check
            if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P) {
                if (DEBUG_LEVEL >= 4) {
                    printf("- Sender needs latest entry, transmitting ... ");
                    fflush(stdout);
                }

                // Create new ETR based on latest entry in the log
                entry_transmission_s *netr = etr_new_from_local_entry(overseer,
                                                                      MSG_TYPE_ETR_LOGFIX,
                                                                      overseer->log->next_index - 1);
                if (netr == NULL) {
                    fprintf(stderr,
                            "Failed to create ETR for replying with entry corresponding to next index\n");
                    return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }

                if (DEBUG_LEVEL >= 4) {
                    printf("Done.\n");
                    fflush(stdout);
                }
            } else { // Should not be necessary because of CM check but well who knows
                if (DEBUG_LEVEL >= 4) {
                    printf("- Sender needs latest entry but local is not P\n");
                    fflush(stdout);
                }
                if (cm_sendto(overseer, addr, socklen, MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send CM of type Indicate P as result of check action\n");
                    fflush(stderr);
                    return EXIT_FAILURE;
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
                                                                      cm->next_index);
                if (netr == NULL) {
                    fprintf(stderr,
                            "Failed to create ETR for replying with entry corresponding to next index\n");
                    return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }

                if (DEBUG_LEVEL >= 4) {
                    printf("Done.\n");
                    fflush(stdout);
                }
            } else { // Should not be necessary because of CM check but well who knows
                if (DEBUG_LEVEL >= 4) {
                    printf("- Sender needs entry corresponding to next index but local is not P\n");
                    fflush(stdout);
                }
                if (cm_sendto(overseer, addr, socklen, MSG_TYPE_INDICATE_P) != EXIT_SUCCESS) {
                    fprintf(stderr,
                            "Failed to send CM of type Indicate P as result of check action\n");
                    fflush(stderr);
                    return EXIT_FAILURE;
                }
            }
            break;

        default:
            fprintf(stderr, "Unimplemented check rv %d\n", rv);
            fflush(stderr);
            return EXIT_FAILURE;
            break;
    }

    if (DEBUG_LEVEL >= 4) {
        printf("End of check action ---------------------------------------------------------------\n");
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}

void cm_retransmission_cb(evutil_socket_t fd, short event, void *arg) {
    if (DEBUG_LEVEL >= 3) {
        printf("CM retransmission timed out, reattempting transmission (attempt %d) ... ",
               ((retransmission_cache_s *) arg)->cur_attempts + 1);
        fflush(stdout);
    }
    int success = 1;

    // Send proposition to P
    if (cm_sendto(((retransmission_cache_s *) arg)->overseer,
                  ((retransmission_cache_s *) arg)->addr,
                  ((retransmission_cache_s *) arg)->socklen,
                  ((retransmission_cache_s *) arg)->type)) {
        fprintf(stderr, "Failed retransmitting the control message\n");
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
            fprintf(stderr, "Failed to add the CM retransmission event\n");
            fflush(stderr);
            success = 0;
        }
    }

    if (DEBUG_LEVEL >= 3 && success == 1) {
        printf("Done.\n");
        fflush(stdout);
    }
    return;
}