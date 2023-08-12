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

    for (int i = 0; npstr->next_index - i - 1 > 0 && i < PSTR_NB_ENTRIES; i++) {
        npstr->last_entries[i] = overseer->log->entries[npstr->next_index - i - 1];
    }

    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            npstr->mfs_array[i][j] = overseer->mfs->array[i][j];
        }
    }

    return npstr;
}

void pstr_print(program_state_transmission_s *pstr, FILE *stream, int flags) {
    int nb_entries = (int) (pstr->next_index >= PSTR_NB_ENTRIES + 2 ? PSTR_NB_ENTRIES : pstr->next_index - 2);
    char buf[32];
    host_status_string(buf, pstr->status);

    if ((flags & FLAG_PRINT_SHORT) == FLAG_PRINT_SHORT)
        fprintf(stream, "- PSTR Metadata [HID %d,  %s,  PTerm %d,  NIx %ld,  CIx %ld]\n",
                pstr->id,
                buf,
                pstr->P_term,
                pstr->next_index,
                pstr->commit_index);
    else
        fprintf(stream, "- PSTR Metadata:\n"
                        "   > host_id:          %d\n"
                        "   > status:           %d (%s)\n"
                        "   > P_term:           %d\n"
                        "   > next_index:       %ld\n"
                        "   > commit_index:     %ld\n"
                        "- Latest %d log entries:\n",
                pstr->id,
                pstr->status,
                buf,
                pstr->P_term,
                pstr->next_index,
                pstr->commit_index,
                nb_entries);
    for (int i = 0; i < nb_entries; ++i) {
        log_entry_state_string(buf, pstr->last_entries[i].state);

        if ((flags & FLAG_PRINT_SHORT) == FLAG_PRINT_SHORT) {
            fprintf(stream, "   > Entry #%d [Term %d,  %s,  Row %d,  Col %d,  Val %c]",
                    i,
                    pstr->last_entries[i].term,
                    buf,
                    pstr->last_entries[i].op.row,
                    pstr->last_entries[i].op.column,
                    pstr->last_entries[i].op.newval);
            if (i % 2 == 1 || i == nb_entries - 1)
                printf("\n");
        } else
            fprintf(stream, "   > Entry #%d:\n"
                            "       * term:     %d\n"
                            "       * state:    %d (%s)\n"
                            "       * row:      %d\n"
                            "       * column:   %d\n"
                            "       * value:    %c\n",
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

    if (DEBUG_LEVEL >= 3)
        printf("\n");
    if (DEBUG_LEVEL >= 4)
        pstr_print(npstr, stdout, CSEC_FLAG_DEFAULT);
    else if (DEBUG_LEVEL == 3)
        pstr_print(npstr, stdout, FLAG_PRINT_SHORT);

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
        printf("Received from %s (aka. %s) a PSTR:\n", buf, overseer->hl->hosts[pstr.id].name);
    }
    if (DEBUG_LEVEL >= 4)
        pstr_print(&pstr, stdout, CSEC_FLAG_DEFAULT);
    else if (DEBUG_LEVEL == 3)
        pstr_print(&pstr, stdout, FLAG_PRINT_SHORT);

    if (pstr_actions(overseer, &pstr) != 0)
        debug_log(0, stderr, "\n--- /!\\ --- Attention: PSTR action detected major incoherences. --- /!\\ ---\n\n");

    pstr_reception_init(overseer);
    debug_log(4, stdout,
              "End of PSTR reception callback ----------------------------------------------------------------------------\n\n");

    if (DEBUG_LEVEL == 3)
        printf("\n");
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
    int late_log = false;

    if (MONITORING_LEVEL >= 4)
        printf("\n");
    if (MONITORING_LEVEL >= 3)
        printf("+++++++++++ Start of coherency check +++++++++++\n");
    if (MONITORING_LEVEL >= 4)
        printf("Latest log entries check:\n");
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
            printf("> Entry #%d of the PSTR has log number 0, this is a major incoherence.\n", i);
            continue;
        }

        // If entry term is lower than local
        if (pstr->last_entries[i].term < overseer->log->entries[entry_number].term) {
            if (MONITORING_LEVEL >= 3) {
                printf("> Entry #%d of the PSTR has term %d instead of %d, this is a minor incoherence.\n",
                       i,
                       pstr->last_entries[i].term,
                       overseer->log->entries[entry_number].term);
            }
            minor_incoherences++;
            continue;
        }

        // If entry term is greater than local
        if (pstr->last_entries[i].term > overseer->log->entries[entry_number].term) {
            printf("> Entry #%d of the PSTR has term %d instead of %d, this is a major incoherence.\n",
                   i,
                   pstr->last_entries[i].term,
                   overseer->log->entries[entry_number].term);
            major_incoherences++;
            continue;
        }

        // If the entry is committed, incoherences are major incoherences, otherwise they're minor incoherences and do
        // not require to be printed at a low verbosity debug level, limiting output clutter.
        if (pstr->last_entries[i].state == ENTRY_STATE_COMMITTED)
            committed = true;
        else committed = false;

        uint8_t op_comp = op_compare(&overseer->log->entries[entry_number].op,
                                     &pstr->last_entries[i].op,
                                     stdout);

        if (op_comp != CSEC_FLAG_DEFAULT) {
            if (MONITORING_LEVEL >= (committed == true ? 1 : 3))
                printf("> Entry #%d ", i);

            // Major incoherence if entry is different and committed or different and has the same term
            if (committed == true || overseer->log->entries[entry_number].term != pstr->last_entries[i].term) {
                if (MONITORING_LEVEL >= 1)
                    printf("    > As the op is committed, this is a major incoherence.\n");
                major_incoherences++;
            } else if (committed == false) {
                if (MONITORING_LEVEL >= 3)
                    printf("    > As the op is uncommitted, this is a minor incoherence.\n");
                minor_incoherences++; // Otherwise if it's different it is only a minor incoherence
            }
        }
    }

    // Check if dist has a more advanced log than local. It should be a major incoherence if dist isn't P (and a minor
    // one otherwise), however a race condition between the messages sent from the different nodes may lead to non-P
    // nodes having a more advanced log. Therefore, we should treat advanced log entries as minor incoherences for all
    // node statuses. Furthermore, in this prototype it should not be possible for an S node to wrongfully commit data
    // this way.
    // TODO Improvement find a way to guard against the above
    int is_p = pstr->status == HOST_STATUS_P ? true : false;
    if (pstr->nb_ops > overseer->mfs->nb_ops) {
        advanced_log = true;
        minor_incoherences++;

        if (MONITORING_LEVEL >= 3) {
            printf("> Dist host's MFS has more applied op (%ld) than local (%ld)%s. This is a minor incoherence.\n",
                   pstr->nb_ops,
                   overseer->mfs->nb_ops,
                   is_p == true ? ", but has status P" : " and hasn't P status");
        }
    }
    // Same but to see if it's outdated compared to local
    if (pstr->nb_ops < overseer->mfs->nb_ops) {
        late_log = true;
        if (is_p == false) minor_incoherences++;
        else major_incoherences++;

        if ((is_p == false && MONITORING_LEVEL >= 3) || MONITORING_LEVEL >= 1) {
            printf("> Dist host's MFS has fewer applied op (%ld) than local (%ld)%s. This is a %s incoherence.\n",
                   pstr->nb_ops,
                   overseer->mfs->nb_ops,
                   is_p == true ? ", but has status P" : " and hasn't P status",
                   is_p == true ? "major" : "minor");
        }
    }

    if (MONITORING_LEVEL >= 4)
        printf("\nMFS Array check:\n");
    int changes = false;
    // Coherency check for the PSTR's MFS array, using local as ref
    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            int difference = pstr->mfs_array[i][j] != overseer->mfs->array[i][j] ? true : false;

            // Regular check if dist host does not have a more advanced or late log
            if (difference == true && advanced_log == false && late_log == false) {
                major_incoherences++;
                changes = true;
                if (MONITORING_LEVEL >= 1) {
                    printf("> Major incoherence at row %d column %d: is '%c' but should be '%c'\n",
                           i,
                           j,
                           pstr->mfs_array[i][j],
                           overseer->mfs->array[i][j]);
                }
            }

            // If dist has a more advanced log, we need to check if there are changes in the log concerning the
            // current MFS cell amongst its advanced entries in the PSTR.
            if (advanced_log == true && difference == true) {
                uint8_t newchar = overseer->mfs->array[i][j];
                for (int k = 0; k < nb_entries; ++k) {
                    // If an entry amongst the advanced ones concerns the cell, use its value for comparison
                    if (pstr->last_entries[k].op.column == j &&
                        pstr->last_entries[k].op.row == i &&
                        advanced_entries[k] == true) {
                        newchar = pstr->last_entries[k].op.newval;
                        break; // We only use the most recent change
                    }
                }
                if (pstr->mfs_array[i][j] != newchar) {
                    major_incoherences++;
                    changes = true;
                    if (MONITORING_LEVEL >= 1) {
                        printf("> Major incoherence at row %d column %d: is '%c' but should be '%c'\n",
                               i,
                               j,
                               pstr->mfs_array[i][j],
                               newchar);
                    }
                }
            }

            // If dist has a late log, we need to check if there are changes in the log concerning the
            // current MFS cell amongst local entries that are not committed yet by dist.
            if (late_log == true && difference == true) {
                uint8_t newchar = pstr->mfs_array[i][j];
                uint64_t nb_missing_commits = overseer->log->commit_index - pstr->commit_index;
                for (uint64_t k = 0; k < nb_missing_commits; ++k) {
                    // If an entry amongst the advanced ones concerns the cell, use its value for comparison
                    if (overseer->log->entries[overseer->log->commit_index - k].op.column == j &&
                        overseer->log->entries[overseer->log->commit_index - k].op.row == i) {
                        newchar = overseer->log->entries[overseer->log->commit_index - k].op.newval;
                        break; // We only use the most recent change
                    }
                }
                if (overseer->mfs->array[i][j] != newchar) {
                    major_incoherences++;
                    changes = true;
                    if (MONITORING_LEVEL >= 1) {
                        printf("> Major incoherence at row %d column %d: is '%c' but should be '%c'\n",
                               i,
                               j,
                               pstr->mfs_array[i][j],
                               newchar);
                    }
                } else { // If dist log is outdated, it is still a minor incoherence
                    minor_incoherences++;
                    changes = true;
                    if (MONITORING_LEVEL >= 3) {
                        printf("> Minor incoherence at row %d column %d: is '%c' but should later become '%c'\n",
                               i,
                               j,
                               pstr->mfs_array[i][j],
                               newchar);
                    }
                }
            }
        }
    }
    if (changes == true && MONITORING_LEVEL >= 4)
        printf("> MFS array OK.\n");

    if ((major_incoherences > 0 || minor_incoherences > 0) && MONITORING_LEVEL >= 1) {
        if (MONITORING_LEVEL >= 3) printf("Coherency check results: \n");
        printf("- Detected a total of %ld major and %ld minor (recoverable) incoherences.\n",
               major_incoherences,
               minor_incoherences);
    } else if (MONITORING_LEVEL >= 2) {
        if (MONITORING_LEVEL >= 3) printf("\nCoherency check results: \n");
        printf("- No incoherences detected.\n");
    }

    if (MONITORING_LEVEL >= 3)
        printf("++++++++++++ End of coherency check ++++++++++++\n");
    if (INSTANT_FFLUSH) fflush(stdout);
    return major_incoherences;
}