//
// Created by zheraan on 05/05/23.
//

#ifndef THESIS_CSEC_TIMEOUT_H
#define THESIS_CSEC_TIMEOUT_H

#include <event2/util.h>

enum timeout_type{
    TIMEOUT_P_HB = 0,
    TIMEOUT_HS_HB = 1,
    TIMEOUT_PROPOSITION = 2,
    TIMEOUT_ACK = 3,
    TIMEOUT_ELECTION = 4,
};

struct timeval *timeout_random();

#endif //THESIS_CSEC_TIMEOUT_H
