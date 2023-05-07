#include "log.h"

log_s * log_init(log_s *log){
    log->next_index = 0;
    log->rep_index = 0;
    log->match_index = 0;
    log->P_term = 0;
    log->HS_term = 0;
    log->nb_entries = 0;

    return log;
}