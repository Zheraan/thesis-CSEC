//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_DATA_OP_H
#define THESIS_CSEC_DATA_OP_H

#include <event2/util.h>
#include "mfs-datatypes.h"
#include "mocked-fs.h"

// Returns a newly allocated data op struct initialized with random values, or NULL in case malloc failed.
data_op_s *op_new();

// Prints a data op structure to the standard output
void op_print(const data_op_s *op, FILE *stream);

#endif //THESIS_CSEC_DATA_OP_H
