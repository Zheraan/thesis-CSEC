#include "log.h"

log_s *log_init(log_s *log) {
    log->next_index = 1;
    log->commit_index = 0;
    log->P_term = 0;
    log->HS_term = 0;
    log->fix_conversation = 0;
    log->log_coherency_counter = 0;
    log->fix_type = FIX_TYPE_NONE;
    // Empty entry state value is 0 so this sets it correctly, as well as the other fields
    memset(log->entries, 0, LOG_LENGTH * sizeof(log_entry_s));

    return log;
}

void log_print(log_s *log, FILE *stream) {
    fprintf(stream,
            "Next index:        %ld\n"
            "Commit index:      %ld\n"
            "P-term:            %d\n"
            "HS-term:           %d\n"
            "Server majority:   %d\n"
            "Master majority:   %d\n",
            log->next_index,
            log->commit_index,
            log->P_term,
            log->HS_term,
            log->server_majority,
            log->master_majority);
    return;
}

void log_entry_state_string(char *buf, enum entry_state state) {
    switch (state) {
        case ENTRY_STATE_INVALID:
            sprintf(buf, "INVALID");
            break;
        case ENTRY_STATE_EMPTY:
            sprintf(buf, "EMPTY");
            break;
        case ENTRY_STATE_PROPOSAL:
            sprintf(buf, "PROPOSAL");
            break;
        case ENTRY_STATE_PENDING:
            sprintf(buf, "PENDING");
            break;
        case ENTRY_STATE_COMMITTED:
            sprintf(buf, "COMMITTED");
            break;
        case ENTRY_STATE_CACHED:
            sprintf(buf, "CACHED");
            break;
    }
    return;
}

int log_add_entry(overseer_s *overseer, const entry_transmission_s *etr, enum entry_state state) {
    if (overseer->log->next_index == LOG_LENGTH) {
        debug_log(0, stderr, "Fatal error: Log full.\n");
        exit(EXIT_FAILURE);
    }

    if (overseer->log->entries[etr->index].state == ENTRY_STATE_COMMITTED) {
        debug_log(2, stdout, "Local entry in given index is already committed, aborting.\n");
        return EXIT_FAILURE;
    }

    // Set entry in the log
    log_entry_s *nentry = &(overseer->log->entries[etr->index]);
    nentry->term = etr->term;
    nentry->state = state;
    nentry->server_rep = 0;
    nentry->master_rep = 0;
    nentry->op.newval = etr->op.newval;
    nentry->op.row = etr->op.row;
    nentry->op.column = etr->op.column;

    char state_string[32];
    log_entry_state_string(state_string, state);
    if (DEBUG_LEVEL >= 4) {
        printf("Added to the log the following entry:\n"
               "- entry number: %ld\n"
               "- P-term:       %d\n"
               "- state:        %d (%s)\n"
               "- row:          %d\n"
               "- column:       %d\n"
               "- newval:       %c\n",
               etr->index,
               nentry->term,
               nentry->state,
               state_string,
               nentry->op.row,
               nentry->op.column,
               nentry->op.newval);
    } else if (DEBUG_LEVEL >= 1) {
        printf("New log entry%s [# %ld,  PTerm %d,  %s,  Row %d,  Col %d,  Val %c]\n",
               DEBUG_LEVEL == 3 ? ":\n-" : "",
               etr->index,
               nentry->term,
               state_string,
               nentry->op.row,
               nentry->op.column,
               nentry->op.newval);
    }

    if (state == ENTRY_STATE_COMMITTED) {
        debug_log(2, stdout, "Entry state is COMMITTED: committing entries up to this point.\n");
        if (log_commit_upto(overseer, etr->index) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failure committing all necessary entries.\n");
            return EXIT_FAILURE;
        }
    }

    if ((state == ENTRY_STATE_PENDING || state == ENTRY_STATE_COMMITTED) && etr->index > overseer->log->next_index) {
        debug_log(4, stdout, "Adjusting next index.\n");
        overseer->log->next_index = etr->index;
    }

    // If local status is a master node and the entry isn't committed, update the replication array. If it was
    // committed, the log_commit_upto will already have updated the array
    if (overseer->hl->hosts[overseer->hl->localhost_id].type == NODE_TYPE_M &&
        state != ENTRY_STATE_COMMITTED)
        hl_replication_index_change(overseer,
                                    overseer->hl->localhost_id,
                                    overseer->log->next_index,
                                    overseer->log->commit_index);

    // If local host is not a cluster monitor and there are some cluster monitors in the list
    if (overseer->hl->nb_monitors > 0 && overseer->hl->hosts[overseer->hl->localhost_id].type != NODE_TYPE_CM) {
        // Increment coherency check counter
        overseer->log->log_coherency_counter++;

        // If Threshold is reached
        if (overseer->log->log_coherency_counter >= COHERENCY_CHECK_THRESHOLD) {
            // Transmit current state
            if (pstr_transmit(overseer) != EXIT_SUCCESS) {
                debug_log(0, stderr, "Failed to transmit at least one PSTR.\n");
            }
            // Then reset counter
            overseer->log->log_coherency_counter = 0;
        }
    }

    return EXIT_SUCCESS;
}

log_entry_s *log_get_entry_by_id(log_s *log, uint64_t id) {
    if (id >= LOG_LENGTH || log->entries[id].state == ENTRY_STATE_EMPTY) return NULL;
    return &(log->entries[id]);
}

int log_repair(overseer_s *overseer, control_message_s *cm) {
    debug_log(1, stdout, "Starting Log Repair ... ");
    uint64_t old_index = overseer->log->next_index;
    // Adjust local next index based on dist and invalidate anything above
    log_invalidate_from(overseer->log, cm->next_index);

    switch (overseer->log->fix_type) {
        case FIX_TYPE_REPAIR:
            // If there is another repair conversation ongoing
            if (cm->ack_back != overseer->log->fix_conversation) {
                // Ignore repair request to not lead to proliferation
                debug_log(1,
                          stdout,
                          "Log Repair requested but already ongoing through another conversation, ignoring action.\n");
                return EXIT_SUCCESS;
            }
                    __attribute__ ((fallthrough));

        case FIX_TYPE_REPLAY:
            // Override current Replay if any
            log_fix_end(overseer);
                    __attribute__ ((fallthrough));

        case FIX_TYPE_NONE:
            // Set the current fix to repair
            overseer->log->fix_type = FIX_TYPE_REPAIR;
            break;

        default:
            debug_log(0, stderr, "Failure: invalid Fix Type.\n");
            return EXIT_FAILURE;
    }

    // If invalidation did not reduce index below the old index
    if (overseer->log->next_index >= old_index) {
        // Decrement next index (with boundary check)
        overseer->log->next_index = overseer->log->next_index > 1 ? overseer->log->next_index - 1 : 1;
        // If the previous entry is marked invalid (for example, because of the logfix carrying an invalidating value in the
        // prev_term field, we further decrease the local next index (also checking for boundaries, the log entries array
        // starting at 1)
        if (overseer->log->next_index - 1 > 2 &&
            overseer->log->entries[overseer->log->next_index - 1].state == ENTRY_STATE_INVALID)
            overseer->log->next_index--;
    }

    // Determine if P is known
    errno = 0;
    uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
    if (p_id == EXIT_FAILURE && errno == ENONE) {
        debug_log(0,
                  stderr,
                  "No active P is known, log repair failed consequently as no fix conversation can be started.\n");
        return EXIT_FAILURE;
    }

    // Create a retransmission cache for the Fix conversation
    uint32_t rtc_id = rtc_add_new(overseer,
                                  CM_DEFAULT_RT_ATTEMPTS,
                                  overseer->hl->hosts[p_id].addr,
                                  overseer->hl->hosts[p_id].socklen,
                                  MSG_TYPE_LOG_REPAIR,
                                  NULL,
                                  cm->ack_reference);
    if (rtc_id == 0) {
        debug_log(0, stderr, "Failed creating retransmission cache for Log Repair.\n");
        return EXIT_FAILURE;
    }

    // Save the fix conversation id
    overseer->log->fix_conversation = rtc_id;

    // Request Logfix through Log Repair
    if (cm_sendto_with_rt_init(overseer,
                               overseer->hl->hosts[p_id].addr,
                               overseer->hl->hosts[p_id].socklen,
                               MSG_TYPE_LOG_REPAIR,
                               0,
                               rtc_id,
                               cm->ack_reference, CSEC_FLAG_DEFAULT) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed to Log Repair send and RT init.\n");
        return EXIT_FAILURE;
    }

    debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int log_replay(overseer_s *overseer, control_message_s *cm) {
    debug_log(1, stdout, "Starting Log Replay ... ");

    if (cm->next_index == 1) {
        debug_log(3, stdout, "Dist log is empty, acknowledging and setting correct term.\n");
        overseer->log->P_term = cm->P_term;
        log_fix_end(overseer);
        return EXIT_SUCCESS;
    }

    switch (overseer->log->fix_type) {
        case FIX_TYPE_REPLAY:
            // If there is another replay conversation ongoing
            if (cm->ack_back != overseer->log->fix_conversation) {
                // Ignore replay request to not lead to proliferation
                debug_log(1,
                          stdout,
                          "Log Replay requested but already ongoing through another conversation, ignoring action.\n");
                return EXIT_SUCCESS;
            }
                    __attribute__ ((fallthrough));

        case FIX_TYPE_REPAIR:
            // If there is a repair conversation ongoing
            log_fix_end(overseer);
                    __attribute__ ((fallthrough));

        case FIX_TYPE_NONE:
            overseer->log->fix_type = FIX_TYPE_REPLAY;
            break;

        default:
            debug_log(0, stderr, "Failure: invalid Fix Type.\n");
            return EXIT_FAILURE;
    }

    // Determine if P is known
    errno = 0;
    uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
    if (p_id == EXIT_FAILURE && errno == ENONE) {
        debug_log(0, stderr, "Log replay failed consequently.\n");
        return EXIT_FAILURE;
    }

    // Create a retransmission cache for the Fix conversation
    debug_log(4, stdout, "Creating new RTC for fix conversation :\n");
    uint32_t rv = rtc_add_new(overseer,
                              CM_DEFAULT_RT_ATTEMPTS,
                              overseer->hl->hosts[p_id].addr,
                              overseer->hl->hosts[p_id].socklen,
                              MSG_TYPE_LOG_REPLAY,
                              NULL,
                              cm->ack_reference);
    if (rv == 0) {
        debug_log(0, stderr, "Failed creating retransmission cache for Log Replay.\n");
        return EXIT_FAILURE;
    }

    // Save the fix conversation id
    overseer->log->fix_conversation = rv;

    // Request Logfix through Log Replay
    if (cm_sendto_with_rt_init(overseer,
                               overseer->hl->hosts[p_id].addr,
                               overseer->hl->hosts[p_id].socklen,
                               MSG_TYPE_LOG_REPLAY,
                               0,
                               rv,
                               cm->ack_reference,
                               CSEC_FLAG_DEFAULT) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed to Log Replay send and RT init.\n");
        return EXIT_FAILURE;
    }

    debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void log_fix_end(overseer_s *overseer) {
    debug_log(4, stdout, "Clearing any fix conversation related RTC entry ... ");
    rtc_remove_by_id(overseer, overseer->log->fix_conversation, FLAG_SILENT);
    overseer->log->fix_conversation = 0;
    overseer->log->fix_type = FIX_TYPE_NONE;
}

int log_repair_ongoing(overseer_s *overseer) {
    if (overseer->log->fix_type == FIX_TYPE_REPAIR)
        return true;
    return false;
}

int log_replay_ongoing(overseer_s *overseer) {
    if (overseer->log->fix_type == FIX_TYPE_REPLAY)
        return true;
    return false;
}

int log_repair_override(overseer_s *overseer, control_message_s *cm) {
    debug_log(0, stderr,
              "Log Repair Override not implemented. This is an optimization for future work. Using Log Repair"
              "instead.\n");
    log_repair(overseer, cm);
    // TODO Extension implement log repair Override
    return EXIT_SUCCESS;
}

void log_invalidate_from(log_s *log, uint64_t index) {
    if (index > log->next_index - 1)
        return;
    uint64_t invalidated = 0;
    uint64_t i = index;
    // It is necessary to guard against indexes below the commit index in order to not invalidate non-erroneous
    // committed entries when an outdated message is received
    if (index <= log->commit_index) {
        i = log->commit_index + 1;
        debug_log(2,
                  stdout,
                  "Invalidating index was lower than local commit index, adjusting to prevent corruption.\n");
    }
    for (; log->entries[i].state != ENTRY_STATE_EMPTY; ++i) {
        if (log->entries[i].state == ENTRY_STATE_COMMITTED) {
            debug_log(0, stderr, "Fatal Error: trying to invalidate COMMITTED log entry.\n");
            exit(EXIT_FAILURE);
        }
        log->entries[i].state = ENTRY_STATE_INVALID;
        invalidated++;
    }
    if (log->next_index > index && index > log->commit_index)
        log->next_index = index;
    if (DEBUG_LEVEL >= 2 & invalidated != 0) {
        printf("Invalidated %ld local log entries from index %ld.\n", invalidated, index);
    }
    return;
}

int log_entry_commit(overseer_s *overseer, uint64_t index) {
    // Fails if entry is marked empty or invalid
    if (overseer->log->entries[index].state == ENTRY_STATE_EMPTY ||
        overseer->log->entries[index].state == ENTRY_STATE_INVALID) {
        fprintf(stderr,
                overseer->log->entries[index].state == ENTRY_STATE_EMPTY ?
                "Failed to commit entry %ld: it is marked as empty.\n" :
                "Failed to commit entry %ld: it is marked as invalid.\n",
                index);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    // Verify that previous entry is also committed
    if (index > 1 && overseer->log->commit_index < index - 1) {
        fprintf(stderr, "Failed to commit entry %ld: previous entry is not in a committed state.\n", index);
        if (INSTANT_FFLUSH) fflush(stderr);
        return EXIT_FAILURE;
    }

    overseer->log->entries[index].state = ENTRY_STATE_COMMITTED;
    if (overseer->log->entries[index].term > overseer->log->P_term)
        overseer->log->P_term = overseer->log->entries[index].term; // Adjust local P-term if necessary

    if (overseer->log->commit_index < index)
        overseer->log->commit_index = index; // Adjust local commit index

    if (overseer->log->next_index <= overseer->log->commit_index)
        overseer->log->next_index = overseer->log->commit_index + 1; // Adjust local next index if necessary

    return mfs_apply_op(overseer->mfs, &overseer->log->entries[index].op); // Apply op in the MFS
}

int log_commit_upto(overseer_s *overseer, uint64_t index) {
    if (index < overseer->log->commit_index)
        return EXIT_SUCCESS;

    if (DEBUG_LEVEL >= 4)
        fprintf(stdout,
                "Committing entries from index %ld up to %ld included ... ",
                overseer->log->commit_index + 1,
                index);

    uint64_t i = 0;
    for (; i + 1 + overseer->log->commit_index <= index; i++) {
        if (log_entry_commit(overseer, i + 1 + overseer->log->commit_index) != EXIT_SUCCESS) {
            if (DEBUG_LEVEL >= 4) {
                fprintf(stdout,
                        "Failure (%ld entr%s successfully committed beforehand).\n",
                        i,
                        i == 1 ? "y" : "ies");
                mfs_array_print(overseer->mfs, stdout);
            }
            return EXIT_FAILURE;
        }
    }

    if (DEBUG_LEVEL >= 4) {
        fprintf(stdout, "Done (%ld entr%s successfully committed).\n",
                i,
                i == 1 ? "y" : "ies");
        mfs_array_print(overseer->mfs, stdout);
    }

    // Adjust replication index
    if (overseer->hl->hosts[overseer->hl->localhost_id].type == NODE_TYPE_M)
        hl_replication_index_change(overseer,
                                    overseer->hl->localhost_id,
                                    overseer->log->next_index,
                                    overseer->log->commit_index);
    return EXIT_SUCCESS;
}