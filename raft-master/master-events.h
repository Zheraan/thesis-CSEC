//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_MASTER_EVENTS_H
#define THESIS_CSEC_MASTER_EVENTS_H

#include <stdint.h>
#include <stdio.h>
#include <event2/util.h>
#include <event2/event.h>
#include "hosts-list/host.h"
#include "data-op.h"
#include "overseer.h"
#include "raft-comms/heartbeat.h"

// Broadcasts a heartbeat to all S and HS if local is P, and to all CS if local is HS
void heartbeat_broadcast(evutil_socket_t sender, short event, void *arg);

#endif //THESIS_CSEC_MASTER_EVENTS_H
