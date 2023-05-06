//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_DATA_OP_H
#define THESIS_CSEC_DATA_OP_H

typedef struct data_op_s data_op_s;

#include <stdint.h>
#include <event2/util.h>
#include "mocked-fs.h"

typedef struct data_op_s{
    // TODO
    uint64_t row;
    uint64_t column;
    int newval;
} data_op_s;

// Returns a newly allocated data op struct initialized with random values, or NULL in case malloc failed
data_op_s *op_new();

#endif //THESIS_CSEC_DATA_OP_H
