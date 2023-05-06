//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_OPS_QUEUE_H
#define THESIS_CSEC_OPS_QUEUE_H

#include <event2/event.h>
#include "data-op.h"
#include "mocked-fs.h"

typedef struct ops_queue_s{
    data_op_s *op;
    struct ops_queue_s *next; // FIFO queue of data ops
    struct event *timeout_event;
} ops_queue_s;

// Frees the memory of a queue element and its timeout event but not its data op
void ops_queue_element_free(ops_queue_s *element);

// Frees the pointed element in the queue and all subsequent ones, their timeout events and their data ops
void ops_queue_free_all(ops_queue_s *queue);

// Removes the first element in the queue to and returns it, or returns NULL if the queue is empty
ops_queue_s *ops_queue_pop(mocked_fs_s *mfs);

// Allocates a new queue element and sets it data op to the one in parameter, and adds it to the queue in the mfs
// Returns a pointer on the newly allocated element or NULL in case of failure
ops_queue_s *ops_queue_add(data_op_s *op, mocked_fs_s *mfs);

#endif //THESIS_CSEC_OPS_QUEUE_H
