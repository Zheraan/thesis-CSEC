//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_SERVER_EVENTS_H
#define THESIS_CSEC_SERVER_EVENTS_H

#include <event2/event.h>
#include "overseer.h"
#include "raft-comms/heartbeat.h"

int server_reception_init(overseer_s *overseer);

#endif //THESIS_CSEC_SERVER_EVENTS_H
