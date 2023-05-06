//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_OPS_QUEUE_H
#define THESIS_CSEC_OPS_QUEUE_H

#include "data-op.h"

typedef struct ops_queue_s{
    data_op_s *op;
    struct ops_queue_s *next; // FIFO queue of data ops
} ops_queue_s;

// Removes the first element in the queue to and returns it, or returns NULL if the queue is empty
ops_queue_s *ops_queue_pop(mocked_fs_s *mfs);

// Allocates a new queue element and sets it data op to the one in parameter, and adds it to the queue in the mfs
// Returns a pointer on the newly allocated element or NULL in case of failure
ops_queue_s *ops_queue_add(data_op_s *op, mocked_fs_s *mfs);

// Frees the memory of a queue element and its data op
void ops_queue_element_free(ops_queue_s *element);

#endif //THESIS_CSEC_OPS_QUEUE_H
