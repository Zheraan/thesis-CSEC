//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_MASTER_EVENTS_H
#define THESIS_CSEC_MASTER_EVENTS_H

#ifndef MASTER_HEARTBEAT_TIMEOUT_SEC
#define MASTER_HEARTBEAT_TIMEOUT_SEC 2 // tv_sec component of the heartbeat timeout
#endif

#ifndef MASTER_HEARTBEAT_TIMEOUT_USEC
#define MASTER_HEARTBEAT_TIMEOUT_USEC 0 // tv_usec component of the heartbeat timeout
#endif

#include "raft-comms/control-message.h"
#include "overseer.h"

// Broadcasts a heartbeat to all S and HS if local is P, and to all CS if local is HS
void master_heartbeat_broadcast_cb(evutil_socket_t sender, short event, void *arg);

int master_heartbeat_init(overseer_s *overseer);

#endif //THESIS_CSEC_MASTER_EVENTS_H
