//
// Created by zheraan on 06/05/23.
//

#ifndef THESIS_CSEC_OPS_QUEUE_H
#define THESIS_CSEC_OPS_QUEUE_H

typedef struct ops_queue_s ops_queue_s;

#include <event2/event.h>
#include "mfs-datatypes.h"
#include "data-op.h"
#include "mocked-fs.h"
#include "../overseer.h"

// Frees the memory of a queue element, its data op, and its timeout event
void ops_queue_element_free(ops_queue_s *element);

// Frees the memory of the first element in the queue, its data op, and its timeout event
void ops_queue_element_free_first(mocked_fs_s *mfs);

// Frees the pointed element in the queue and all subsequent ones, their timeout events and their data ops.
// Returns the number of element that were deleted, or -1 in case of error.
int ops_queue_free_all(overseer_s *overseer, ops_queue_s *element);

// Removes the first element in the queue to and returns it, or returns NULL if the queue is empty
ops_queue_s *ops_queue_pop(mocked_fs_s *mfs);

// Allocates a new queue element and sets it data op to the one in parameter, and adds it to the queue in the mfs
// Returns a pointer on the newly allocated element or NULL in case of failure
ops_queue_s *ops_queue_add(data_op_s *op, mocked_fs_s *mfs);

#endif //THESIS_CSEC_OPS_QUEUE_H
