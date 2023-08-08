//
// Created by zheraan on 06/08/23.
//

#include "state-monitoring.h"

program_state_transmission_s *pstr_new(overseer_s *overseer) {
    program_state_transmission_s *npstr = malloc(sizeof(program_state_transmission_s));
    if (npstr == NULL) {
        perror("malloc npstr");
        return NULL;
    }
    npstr->id = overseer->hl->localhost_id;
    npstr->P_term = overseer->log->P_term;
    npstr->next_index = overseer->log->next_index;
    npstr->commit_index = overseer->log->commit_index;
    npstr->nb_ops = overseer->mfs->nb_ops;
    npstr->status = overseer->hl->hosts[overseer->hl->localhost_id].status;

    for (int i = 0; npstr->next_index - i - 1 > 0; i++) {
        npstr->last_entries[i] = overseer->log->entries[npstr->next_index - i - 1];
    }

    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            npstr->mfs_array[i][j] = overseer->mfs->array[i][j];
        }
    }

    return npstr;
}

void pstr_print(overseer_s *overseer, program_state_transmission_s *pstr, FILE *stream) {
    int nb_entries = (int) (pstr->next_index >= PSTR_NB_ENTRIES + 2 ? PSTR_NB_ENTRIES : pstr->next_index - 2);
    char buf[20];
    host_status_string(buf, pstr->status);
    fprintf(stream, "- PSTR Metadata:\n"
                    "   > id:               %d (aka. %s)\n"
                    "   > status:           %d (%s)\n"
                    "   > P-term:           %d\n"
                    "   > next index:       %ld\n"
                    "   > commit index:     %ld\n"
                    "- Latest %d log entries:\n",
            pstr->id,
            overseer == NULL ? "?" : overseer->hl->hosts[pstr->id].name,
            pstr->status,
            buf,
            pstr->P_term,
            pstr->next_index,
            pstr->commit_index,
            nb_entries);
    for (int i = 0; i < nb_entries; ++i) {
        log_entry_state_string(buf, pstr->last_entries[i].term);
        fprintf(stream, "   > entry #%d:\n"
                        "       * term:     %d\n"
                        "       * state:    %d (%s)\n"
                        "       * row:      %d\n"
                        "       * column:   %d\n"
                        "       * value:    %d\n",
                i,
                pstr->last_entries[i].term,
                pstr->last_entries[i].state,
                buf,
                pstr->last_entries[i].op.row,
                pstr->last_entries[i].op.column,
                pstr->last_entries[i].op.newval);
    }
    fprintf(stream, "- MFS array contents:\n");
    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            fprintf(stream, "%c  ", pstr->mfs_array[i][j]);
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "   > Number of applied Ops:   %ld\n", pstr->nb_ops);

    if (INSTANT_FFLUSH) fflush(stream);
    return;
}

int pstr_transmit(overseer_s *overseer) {
    debug_log(3, stdout, "Transmitting program state ... ");

    program_state_transmission_s *npstr = pstr_new(overseer);
    if (npstr == NULL) {
        debug_log(0, stderr, "Allocating program state structure failed.\n");
        return EXIT_FAILURE;
    }
    int rv = EXIT_SUCCESS;

    if (DEBUG_LEVEL >= 4)
        pstr_print(overseer, npstr, stdout);

    host_s *target;
    struct sockaddr_in6 target_addr;
    socklen_t target_socklen;
    int nb_pstr = 0;
    for (uint32_t i = 0; i < overseer->hl->nb_hosts; i++) {
        target = &(overseer->hl->hosts[i]);

        // Skip if target is not a cluster monitor
        if (target->type != NODE_TYPE_CM)
            continue;

        target_addr = (target->addr);
        target_socklen = (target->socklen);

        nb_pstr++;
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(target_addr.sin6_addr), buf, 256);
        if (DEBUG_LEVEL >= 3) {
            printf("PSTR %d target: %s @ %s\n", nb_pstr, target->name, buf);
            if (INSTANT_FFLUSH) fflush(stdout);
        }

        target_addr.sin6_port = htons(PORT_PSTR);

        do {
            errno = 0;
            if (sendto(overseer->socket_cm,
                       npstr,
                       sizeof(program_state_transmission_s),
                       0,
                       (const struct sockaddr *) &target_addr,
                       target_socklen) == -1) {
                perror("PSTR sendto");
                if (errno != EAGAIN)
                    rv = EXIT_FAILURE;
            }
        } while (errno == EAGAIN);
    }

    free(npstr);
    if (DEBUG_LEVEL >= 3) {
        printf("Done (%d PSTRs sent).\n", nb_pstr);
        if (INSTANT_FFLUSH) fflush(stdout);
    }
    return rv;
}

int pstr_reception_init(overseer_s *overseer) {
    debug_log(4, stdout, "Initializing next PSTR reception event ... ");
    struct event *reception_event = event_new(overseer->eb,
                                              overseer->socket_cm,
                                              EV_READ,
                                              pstr_receive_cb,
                                              (void *) overseer);
    if (reception_event == NULL) {
        fprintf(stderr, "Fatal error: failed to create the next PSTR reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // PSTR reception has low priority
    event_priority_set(reception_event, 1);

    if (overseer->special_event != NULL) // Freeing the past event if any
        event_free(overseer->special_event);
    overseer->special_event = reception_event;

    // Add the event in the loop
    if (event_add(reception_event, NULL) != 0) {
        fprintf(stderr, "Fatal error: failed to add the next PSTR reception event\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    debug_log(4, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void pstr_receive_cb(evutil_socket_t fd, short event, void *arg) {
    debug_log(4, stdout,
              "Start of PSTR reception callback -------------------------------------------------------------------------\n");
    overseer_s *overseer = arg;
    program_state_transmission_s pstr;
    struct sockaddr_in6 sender_addr;
    socklen_t socklen = sizeof(sender_addr);

    do {
        errno = 0;
        if (recvfrom(fd,
                     &pstr,
                     sizeof(program_state_transmission_s),
                     0,
                     (struct sockaddr *) &sender_addr,
                     &socklen) == -1) {
            perror("recvfrom PSTR");
            if (errno != EAGAIN)
                return; // Failure
        }
    } while (errno == EAGAIN);

    if (MONITORING_LEVEL >= 1) {
        char buf[256];
        evutil_inet_ntop(AF_INET6, &(sender_addr.sin6_addr), buf, 256);
        printf("Received from %s (aka. %s) a PSTR\n", buf, overseer->hl->hosts[pstr.id].name);
        if (MONITORING_LEVEL >= 3)
            pstr_print(overseer, &pstr, stdout);
    }
    if (DEBUG_LEVEL >= 4)
        pstr_print(overseer, &pstr, stdout);

    if (pstr_actions(overseer, &pstr) != 0)
        debug_log(0, stderr, "PSTR action detected major incoherences.\n");

    pstr_reception_init(overseer);
    debug_log(4, stdout,
              "End of PSTR reception callback ----------------------------------------------------------------------------\n\n");
    return;
}

uint64_t pstr_actions(overseer_s *overseer, program_state_transmission_s *pstr) {
    uint64_t major_incoherences = 0, minor_incoherences = 0;
    int committed;
    // Determine how many log entries are in the PSTR. It may be lower than PSTR_NB_ENTRIES in case the log had fewer
    // entries when the PSTR was sent.
    int nb_entries = (int) (pstr->next_index >= PSTR_NB_ENTRIES + 2 ? PSTR_NB_ENTRIES : pstr->next_index - 2);

    // Array of booleans to mark entries that dist has but not the local monitor, in order to check for coherency with
    // the MFS
    uint8_t advanced_entries[PSTR_NB_ENTRIES] = {false};
    int advanced_log = false;

    // Comparison of the log entries in the PSTR to the local log
    for (int i = 0; i < nb_entries; ++i) {
        // Number of target log entry for the current iteration
        uint64_t entry_number = pstr->next_index - 1 - i;

        // If target entry has higher number than the maximum value in the local log
        if (entry_number > overseer->log->next_index - 1) {
            advanced_entries[i] = true;
            minor_incoherences++;
            continue;
        }
        if (entry_number == 0) {
            major_incoherences++;
            printf("Entry %d of the PSTR has log number 0, this is a major incoherence.\n", i);
            continue;
        }

        // If the entry is committed, incoherences are major incoherences, otherwise they're minor incoherences and do
        // not require to be printed at a low verbosity debug level, limiting output clutter.
        if (pstr->last_entries[entry_number].state == ENTRY_STATE_COMMITTED)
            committed = true;
        else committed = false;

        uint8_t op_comp = op_compare(&overseer->log->entries[entry_number].op,
                                     &pstr->last_entries[entry_number].op,
                                     stdout,
                                     committed == true ? 1 : 3);

        if (op_comp != CSEC_FLAG_DEFAULT && committed == true) {
            major_incoherences++;
        } else if (op_comp != CSEC_FLAG_DEFAULT && committed == false) {
            minor_incoherences++;
        }
    }

    // Check if dist has a more advanced log than local. It should be a major incoherence if dist isn't P (and a minor
    // one otherwise), however a race condition between the messages sent from the different nodes may lead to non-P
    // nodes having a more advanced log. Therefore, we should treat advanced log entries as minor incoherences for all
    // node statuses. Furthermore, in this prototype it should not be possible for an S node to wrongfully commit data
    // this way.
    // TODO Improvement find a way to guard against the above
    if (pstr->nb_ops > overseer->mfs->nb_ops) {
        int is_p = pstr->status == HOST_STATUS_P ? true : false;
        advanced_log = true;
        minor_incoherences++;

        if ((is_p == true && MONITORING_LEVEL >= 3) || MONITORING_LEVEL >= 1) {
            printf("Dist host's MFS has more applied op (%ld) than reference (%ld) %s. This is a %s incoherence.%s\n",
                   pstr->nb_ops,
                   overseer->mfs->nb_ops,
                   is_p == true ? ", but has status P" : "and hasn't P status",
                   "minor",
                   is_p == true ? ""
                                : " However, minor incoherences caused by an advanced MFS may be major incoherences"
                                  " in case this node has committed wrong entries.");
        }
    }

    // Coherency check for the PSTR's MFS array, using local as ref
    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            // Regular check if dist host does not have a more advanced log
            if (advanced_log == false && pstr->mfs_array[i][j] != overseer->mfs->array[i][j]) {
                major_incoherences++;
                if (MONITORING_LEVEL >= 1) {
                    printf("Major incoherence at row %d column %d: is %c but should be %c\n",
                           i,
                           j,
                           pstr->mfs_array[i][j],
                           overseer->mfs->array[i][j]);
                }
            }

            // If dist is P and has a more advanced log, we need to check if there are changes in the log concerning the
            // current MFS cell amongst its advanced entries in the PSTR.
            if (advanced_log == true && pstr->mfs_array[i][j] != overseer->mfs->array[i][j]) {
                uint8_t newchar = overseer->mfs->array[i][j];
                for (int k = 0; k < nb_entries; ++k) {
                    if (pstr->last_entries[k].op.column == j &&
                        pstr->last_entries[k].op.row == i &&
                        advanced_entries[k] == true) {
                        newchar = pstr->last_entries[k].op.newval;
                        break;
                    }
                }
                if (pstr->mfs_array[i][j] != newchar) {
                    major_incoherences++;
                    if (MONITORING_LEVEL >= 1) {
                        printf("Major incoherence at row %d column %d: is %c but should be %c\n",
                               i,
                               j,
                               pstr->mfs_array[i][j],
                               newchar);
                    }
                }
            }
        }
    }

    if (major_incoherences > 0 || minor_incoherences > 0) {
        printf("Detected incoherences: %ld major and %ld minor (recoverable).\n",
               major_incoherences,
               minor_incoherences);
    } else printf("No incoherences detected.\n");
    if (INSTANT_FFLUSH) fflush(stdout);
    return major_incoherences;
}