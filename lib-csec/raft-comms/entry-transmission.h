//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_ENTRY_TRANSMISSION_H
#define THESIS_CSEC_ENTRY_TRANSMISSION_H

#include <stdlib.h>
#include <stdint.h>
#include "control-message.h"

typedef struct transmission_s {
    control_message_s hb; // Status of the sender
    data_op_s op;
    uint32_t index;
    uint32_t term;
    enum entry_state state;
} transmission_s;

#endif //THESIS_CSEC_ENTRY_TRANSMISSION_H
