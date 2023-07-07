#include "log.h"

log_s *log_init(log_s *log) {
    log->next_index = 1;
    log->commit_index = 0;
    log->P_term = 0;
    log->HS_term = 0;
    log->fix_conversation = 0;
    log->fix_type = FIX_TYPE_NONE;
    memset(log->entries, 0, LOG_LENGTH * sizeof(log_entry_s));
    for (int i = 0; i < LOG_LENGTH; ++i) {
        log->entries[i].state = ENTRY_STATE_EMPTY;
    }

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

int log_add_entry(overseer_s *overseer, const entry_transmission_s *etr, enum entry_state state) {
    if (overseer->log->next_index == LOG_LENGTH) {
        fprintf(stderr, "Log full\n");
        return EXIT_FAILURE;
    }

    log_entry_s *nentry = &(overseer->log->entries[etr->index]);
    nentry->term = overseer->log->P_term;
    nentry->state = state;
    nentry->server_rep = 0;
    nentry->master_rep = 0;

    nentry->op.newval = etr->op.newval;
    nentry->op.row = etr->op.row;
    nentry->op.column = etr->op.column;

    if (DEBUG_LEVEL >= 1) {
        printf("Added to the log the following entry:\n"
               "- entry number: %ld\n"
               "- P-term:         %d\n"
               "- state:        %d\n"
               "- row:          %d\n"
               "- column:       %d\n"
               "- newval:       %c\n",
               overseer->log->next_index,
               nentry->term,
               nentry->state,
               nentry->op.row,
               nentry->op.column,
               nentry->op.newval);
    }

    if (state == ENTRY_STATE_COMMITTED) {
        if (log_commit_upto(overseer, etr->index) != EXIT_SUCCESS) {
            debug_log(0, stderr, "Failure committing all necessary entries.\n");
            return EXIT_FAILURE;
        }
    }

    if (state == ENTRY_STATE_PENDING && etr->index == overseer->log->next_index)
        overseer->log->next_index++;
    return EXIT_SUCCESS;
}

log_entry_s *log_get_entry_by_id(log_s *log, uint64_t id) {
    if (id >= LOG_LENGTH || log->entries[id].state == ENTRY_STATE_EMPTY) return NULL;
    return &(log->entries[id]);
}

int log_repair(overseer_s *overseer, control_message_s *cm) {
    debug_log(1, stdout, "Starting Log Repair ... ");
    // Adjust local next index based on dist and invalidate anything above
    log_invalidate_from(overseer->log, cm->next_index);

    switch (overseer->log->fix_type) {
        case FIX_TYPE_REPAIR:
            // If there is another repair conversation ongoing
            if (cm->ack_back != overseer->log->fix_conversation) {
                // Ignore repair request to not lead to proliferation
                debug_log(2,
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
            overseer->log->fix_type = FIX_TYPE_REPAIR;
            break;

        default:
            debug_log(0, stderr, "Failure: invalid Fix Type.\n");
            return EXIT_FAILURE;
    }

    // Decrement next index (with boundary check)
    overseer->log->next_index = overseer->log->next_index > 1 ? overseer->log->next_index - 1 : 1;

    // Determine if P is known
    errno = 0;
    uint32_t p_id = hl_whois(overseer->hl, HOST_STATUS_P);
    if (p_id == EXIT_FAILURE && errno == ENONE) {
        debug_log(0, stderr, "Log repair failed consequently.\n");
        return EXIT_FAILURE;
    }

    // Create a retransmission cache for the Fix conversation
    uint32_t rv = rtc_add_new(overseer,
                              CM_DEFAULT_RT_ATTEMPTS,
                              overseer->hl->hosts[p_id].addr,
                              overseer->hl->hosts[p_id].socklen,
                              MSG_TYPE_LOG_REPAIR,
                              NULL,
                              cm->ack_reference);
    if (rv == 0) {
        debug_log(0, stderr, "Failed creating retransmission cache for Log Repair.\n");
        return EXIT_FAILURE;
    }

    // Save the fix conversation id
    overseer->log->fix_conversation = rv;

    // Request Logfix through Log Repair
    if (cm_sendto_with_rt_init(overseer,
                               overseer->hl->hosts[p_id].addr,
                               overseer->hl->hosts[p_id].socklen,
                               MSG_TYPE_LOG_REPAIR,
                               0,
                               rv,
                               cm->ack_reference) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed to Log Repair send and RT init.\n");
        return EXIT_FAILURE;
    }

    debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

int log_replay(overseer_s *overseer, control_message_s *cm) {
    debug_log(1, stdout, "Starting Log Replay ... ");
    switch (overseer->log->fix_type) {
        case FIX_TYPE_REPLAY:
            // If there is another replay conversation ongoing
            if (cm->ack_back != overseer->log->fix_conversation) {
                // Ignore replay request to not lead to proliferation
                debug_log(2,
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
                               cm->ack_reference) != EXIT_SUCCESS) {
        debug_log(0, stderr, "Failed to Log Replay send and RT init.\n");
        return EXIT_FAILURE;
    }

    debug_log(1, stdout, "Done.\n");
    return EXIT_SUCCESS;
}

void log_fix_end(overseer_s *overseer) {
    rtc_remove_by_id(overseer, overseer->log->fix_conversation, FLAG_SILENT);
    overseer->log->fix_conversation = 0;
    overseer->log->fix_type = FIX_TYPE_NONE;
}

int log_repair_ongoing(overseer_s *overseer) {
    if (overseer->log->fix_type == FIX_TYPE_REPAIR)
        return 1;
    return 0;
}

int log_replay_ongoing(overseer_s *overseer) {
    if (overseer->log->fix_type == FIX_TYPE_REPLAY)
        return 1;
    return 0;
}

int log_repair_override(overseer_s *overseer) {
    debug_log(0, stderr, "Log Repair Override not implemented. This is an optimization for future work.\n");
    return EXIT_SUCCESS;
}

void log_invalidate_from(log_s *log, uint64_t index) {
    for (uint64_t i = index;
         log->entries[i].state != ENTRY_STATE_EMPTY || log->entries[i].state != ENTRY_STATE_COMMITTED;
         ++i) {
        log->entries[i].state = ENTRY_STATE_INVALID;
    }
    if (log->next_index > index)
        log->next_index = index;
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
        fflush(stderr);
        return EXIT_FAILURE;
    }

    if (index > 1 && overseer->log->commit_index < index - 1) {
        fprintf(stderr, "Failed to commit entry %ld: previous entry is not in a committed state.\n", index);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    overseer->log->entries[index].state = ENTRY_STATE_COMMITTED;
    overseer->log->commit_index++;
    if (index == overseer->log->next_index)
        overseer->log->next_index++;
    return mfs_apply_op(overseer->mfs, &overseer->log->entries[index].op);
}

int log_commit_upto(overseer_s *overseer, uint64_t index) {
    if (DEBUG_LEVEL >= 4)
        fprintf(stdout,
                "Committing entries from index %ld up to index %ld ... ",
                overseer->log->commit_index,
                index);
    uint64_t i = 0;
    for (; i + overseer->log->commit_index <= index; i++) {
        if (log_entry_commit(overseer, i + overseer->log->commit_index) != EXIT_SUCCESS) {
            if (DEBUG_LEVEL >= 4)
                fprintf(stdout, "Failure (%ld entries successfully committed).\n", index);
            return EXIT_FAILURE;
        }
    }

    if (DEBUG_LEVEL >= 4)
        fprintf(stdout, "Done (%ld entries successfully committed).\n", index);
    return EXIT_SUCCESS;
}