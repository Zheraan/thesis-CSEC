#include "log.h"

log_s *log_init(log_s *log) {
    log->next_index = 0;
    log->commit_index = 0;
    log->match_index = 0;
    log->P_term = 0;
    log->HS_term = 0;

    return log;
}

int log_add_entry(overseer_s *overseer, const transmission_s *tr, enum entry_state state) {
    if (overseer->log->next_index == LOG_LENGTH) {
        fprintf(stderr, "Log full\n");
        return EXIT_FAILURE;
    }
    log_entry_s *nentry = &(overseer->log->entries[overseer->log->next_index]);
    nentry->term = overseer->log->P_term;
    nentry->state = state;
    nentry->is_cached = 0;
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
    }

    nentry->op.newval = tr->op.newval;
    nentry->op.row = tr->op.row;
    nentry->op.column = tr->op.column;

    if (DEBUG_LEVEL >= 1) {
        printf("Added to the log the following entry:\n"
               "- entry number: %ld\n"
               "- term:         %d\n"
               "- state:        %d\n"
               "- newval:       %d\n"
               "- row:          %ld\n"
               "- column:       %ld\n",
               overseer->log->next_index,
               nentry->term,
               nentry->state,
               nentry->op.newval,
               nentry->op.row,
               nentry->op.column);
    }

    overseer->log->next_index++;
    return EXIT_SUCCESS;
}

void log_free(log_s *log) {
    for (uint64_t i = 0; i < log->next_index; ++i) {
        free(log->entries[i].server_rep);
        free(log->entries[i].master_rep);
    }
    free(log);
    return;
}