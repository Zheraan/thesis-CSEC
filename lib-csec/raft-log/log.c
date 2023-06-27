#include "log.h"

log_s *log_init(log_s *log) {
    log->next_index = 1;
    log->commit_index = 0;
    log->P_term = 0;
    log->HS_term = 0;
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

int log_add_entry(overseer_s *overseer, const entry_transmission_s *tr, enum entry_state state) {
    if (overseer->log->next_index == LOG_LENGTH) {
        fprintf(stderr, "Log full\n");
        return EXIT_FAILURE;
    }
    log_entry_s *nentry = &(overseer->log->entries[tr->index]);
    nentry->term = overseer->log->P_term;
    nentry->state = state;
    nentry->server_rep = 0;
    nentry->master_rep = 0;

    nentry->op.newval = tr->op.newval;
    nentry->op.row = tr->op.row;
    nentry->op.column = tr->op.column;

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

    if (state != ENTRY_STATE_CACHED)
        overseer->log->next_index++;
    return EXIT_SUCCESS;
}

log_entry_s *log_get_entry_by_id(log_s *log, uint64_t id) {
    if (id >= LOG_LENGTH || log->entries[id].state == ENTRY_STATE_EMPTY) return NULL;
    return &(log->entries[id]);
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