//
// Created by zheraan on 27/04/23.
//

#ifndef THESIS_CSEC_OVERSEER_H
#define THESIS_CSEC_OVERSEER_H

#include <event2/event.h>
#include "hosts-list/hosts-list.h"
#include "raft-entry-log/log.h"

typedef struct overseer_s{
    struct event_base *eb;
    hosts_list_s *hl;
    log_s *log;
}overseer_s;

#endif //THESIS_CSEC_OVERSEER_H
