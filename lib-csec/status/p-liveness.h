//
// Created by zheraan on 14/07/23.
//

#ifndef THESIS_CSEC_P_LIVENESS_H
#define THESIS_CSEC_P_LIVENESS_H

#include "status-datatypes.h"
#include "../overseer.h"

// (re)sets the timeout event for P's liveness. Returns EXIT_SUCCESS or causes a Fatal Error in case of failure.
int p_liveness_set_timeout(overseer_s *overseer);

// Callback for when P's liveness times out. Sets the status of P (if any) as Unreachable, then promotes the
// local node to P if local is HS, or sets the next timeout otherwise.
void p_liveness_timeout_cb(evutil_socket_t fd, short event, void *arg);

#endif //THESIS_CSEC_P_LIVENESS_H
