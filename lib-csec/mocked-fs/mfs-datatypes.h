//
// Created by zheraan on 10/05/23.
//

#ifndef THESIS_CSEC_MFS_DATATYPES_H
#define THESIS_CSEC_MFS_DATATYPES_H

#include <stdint.h>

#ifndef MOCKED_FS_ARRAY_COLUMNS
#define MOCKED_FS_ARRAY_COLUMNS 10
#endif

#ifndef MOCKED_FS_ARRAY_ROWS
#define MOCKED_FS_ARRAY_ROWS 2
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

typedef struct data_op_s {
    // Vertical position of the op in the mocked filesystem
    uint8_t row;
    // Horizontal position of the op in the mocked filesystem
    uint8_t column;
    // Character that replaces the previous one at the location indicated by the row and column values
    uint8_t newval;
} data_op_s;

// Queue for data ops on server side in case P is not available for treating proposition, which may happen if another
// proposition is being examined or if there is currently no reachable P
typedef struct ops_queue_s {
    // Pointer to the data op this queue element refers to
    data_op_s *op;
    // FIFO list of data ops
    struct ops_queue_s *next;
    // Pointer to the timeout event related to this queue element, used to free its memory space or disable it in case
    // the queue is emptied or if the data op proposition has been accepted
    struct event *timeout_event;
} ops_queue_s;

typedef struct mocked_fs_s {
    // Array containing the synced data, initialized to all 0
    uint8_t array[MOCKED_FS_ARRAY_ROWS][MOCKED_FS_ARRAY_COLUMNS];
    // Number of ops applied since initialization or last patch
    uint64_t nb_ops;
    ops_queue_s *queue;
} mocked_fs_s;

#endif //THESIS_CSEC_MFS_DATATYPES_H
