//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_MASTER_EVENTS_H
#define THESIS_CSEC_MASTER_EVENTS_H

#include "raft-comms/timeout.h"
#include "raft-comms/control-message.h"
#include "overseer.h"

// Broadcasts a heartbeat to all S and HS if local is P, and to all CS if local is HS
void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg);

int master_heartbeat_init(overseer_s *overseer);

#endif //THESIS_CSEC_MASTER_EVENTS_H
