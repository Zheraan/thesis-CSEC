//
// Created by zheraan on 26/04/23.
//

#include "control-message.h"

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

int cm_sendto(const overseer_s *overseer, struct sockaddr_in6 sockaddr, socklen_t socklen, enum message_type type) {
    control_message_s *cm = cm_new(overseer, type);
    if (cm == NULL) {
        fprintf(stderr, "Failed to create message of type %d", type);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    char buf[256];
    evutil_inet_ntop(AF_INET6, &(sockaddr.sin6_addr), buf, 256);

    if (DEBUG_LEVEL >= 3) {
        printf("Sending to %s the following CM:\n", buf);
        cm_print(cm, stdout);
    }

    int success = 1;
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
            success = 0;
        }
    } while (errno == EAGAIN);

    free(cm);
    if (success) {
        if (DEBUG_LEVEL >= 3) {
            printf("Done.\n");
            fflush(stdout);
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
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
    cm_check_action((overseer_s *) arg, crv); // TODO Failure handling

    // Responses depending on the type of control message
    switch (cm.type) {
        case MSG_TYPE_HB_DEFAULT:
            if (DEBUG_LEVEL >= 2)
                printf("-> is HB DEFAULT\n");
            // Default heartbeat means replying with cm ack
            if (cm_sendto(((overseer_s *) arg),
                          sender,
                          sender_len,
                          MSG_TYPE_ACK_HB) == EXIT_FAILURE) {
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

enum cm_check_rv cm_check_metadata(overseer_s *overseer, const control_message_s *hb) {
    // If local P-term is outdated
    if (hb->term > overseer->log->P_term) {
        // Adjust local P-term
        overseer->log->P_term = hb->term;

        // Indicate P messages have
        if (hb->type != MSG_TYPE_INDICATE_P) {
            // If sender is either P or HS, adjust the identity of HS or P in the hosts list (resetting current P or HS)
            if (hb->status == HOST_STATUS_P || hb->status == HOST_STATUS_HS) {
                if (hl_change_master(overseer->hl, hb->status, hb->host_id) == EXIT_FAILURE) {
                    fprintf(stderr, "CM check metadata error: failed to adjust master status\n");
                    return CHECK_RV_FAILURE;
                }
            }
                // Else set the status of the sender in the hosts list
            else overseer->hl->hosts[hb->host_id].status = hb->status;

            // And ask for repair in case log entries are outdated (cannot rely on indexes only if term was wrong)
            return CHECK_RV_NEED_REPAIR;
        }
        // TODO handle if message is Indicate P
    }

        // If dist P-term is outdated
    else if (hb->term < overseer->log->P_term) {
        if (overseer->hl->hosts[overseer->hl->localhost_id].status != HOST_STATUS_P)
            return CHECK_RV_NEED_INDICATE_P;
        else return CHECK_RV_NEED_TR_LATEST;
    }

    // Else P-terms are equal
    // TODO Implement HS-term
    // If local commit index is outdated (and term and next index were up-to-date)
    if (hb->commit_index > overseer->log->commit_index) {
        // TODO Commit up to commit index
    }
        // Else if dist is outdated do nothing ? TODO check if it is not necessary to notify sender

        // Set the status of the sender in the hosts list
    else overseer->hl->hosts[hb->host_id].status = hb->status; // TODO figure a way to make a security check on this

    // If local next index is outdated
    if (hb->next_index > overseer->log->next_index) {
        // Ask P for the next missing entry
        return CHECK_RV_NEED_REPLAY;
    }

        // If dist next index is outdated
    else if (hb->next_index < overseer->log->next_index) {
        // If local is P
        if (overseer->hl->hosts[overseer->hl->localhost_id].status == HOST_STATUS_P)
            return CHECK_RV_NEED_TR_NEXT; // Send value entry corresponding to sender's next index
        // Else do nothing ? TODO check if it is not necessary to notify sender
    }

    return CHECK_RV_CLEAN;
}

int cm_check_action(overseer_s *overseer, enum cm_check_rv rv) {
    // TODO
    return EXIT_SUCCESS;
}