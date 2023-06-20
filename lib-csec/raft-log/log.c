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

int log_add_entry(overseer_s *overseer, const entry_transmission_s *tr, enum entry_state state) {
    if (overseer->log->next_index == LOG_LENGTH) {
        fprintf(stderr, "Log full\n");
        return EXIT_FAILURE;
    }
    log_entry_s *nentry = &(overseer->log->entries[tr->index]);
    nentry->term = overseer->log->P_term;
    nentry->state = state;
    nentry->server_maj = 0;
    nentry->master_maj = 0;

    // If local host is a master node, allocate replication array for the entry
    if (overseer->hl->hosts[overseer->hl->localhost_id].type == NODE_TYPE_M) {
        nentry->server_rep = malloc(sizeof(uint8_t) * overseer->hl->nb_hosts);
        if (nentry->server_rep == NULL) {
            perror("malloc server rep array for new entry");
            return EXIT_FAILURE; // Abort in case of failure
        }

        nentry->master_rep = malloc(sizeof(uint8_t) * overseer->hl->nb_hosts);
        if (nentry->master_rep == NULL) {
            perror("malloc master rep array for new entry");
            free(nentry->server_rep);
            return EXIT_FAILURE; // Abort in case of failure
        }

        memset(nentry->master_rep, 0, sizeof(uint8_t) * overseer->hl->nb_hosts);
        memset(nentry->server_rep, 0, sizeof(uint8_t) * overseer->hl->nb_hosts);
    }

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

void log_free(log_s *log) {
    for (uint64_t i = 0; i < log->next_index; ++i)
        log_entry_replication_arrays_free(&(log->entries[i]));
    free(log);
    return;
}

void log_entry_replication_arrays_free(log_entry_s *entry) {
    free(entry->server_rep);
    free(entry->master_rep);
    entry->master_rep = NULL;
    entry->server_rep = NULL;
    return;
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
    for (uint64_t i = overseer->log->commit_index; i <= index; i++) {
        if (log_entry_commit(overseer, i) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}