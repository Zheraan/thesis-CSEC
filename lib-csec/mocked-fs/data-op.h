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

// Compares the second op to the first and prints the result of the comparison to the given stream.
// Returns CSEC_FLAG_DEFAULT (zero) if ops are identical, or a bitfield of the following flags:
// - OPCOMP_FLAG_INVALID_ROW
// - OPCOMP_FLAG_INVALID_COLUMN
// - OPCOMP_FLAG_INVALID_VALUE
// See mfs-datatypes.h for the definition of these flags
uint8_t op_compare(data_op_s *ref, data_op_s *val, FILE *stream);

// Prints a data op structure to the standard output
void op_print(const data_op_s *op, FILE *stream);

#endif //THESIS_CSEC_DATA_OP_H
