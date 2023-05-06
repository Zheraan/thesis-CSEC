//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_MOCKED_FS_H
#define THESIS_CSEC_MOCKED_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data-op.h"
#include "ops-queue.h"

#ifndef MOCKED_FS_ARRAY_COLUMNS
#define MOCKED_FS_ARRAY_COLUMNS 10
#endif

#ifndef MOCKED_FS_ARRAY_ROWS
#define MOCKED_FS_ARRAY_ROWS 2
#endif

typedef struct mocked_fs_s{
    int array[MOCKED_FS_ARRAY_ROWS][MOCKED_FS_ARRAY_COLUMNS]; // Array containing the synced data, initialized to all 0
    uint64_t nb_ops; // Number of ops applied since initialization
    ops_queue_s *queue;
} mocked_fs_s;

// Prints the contents of the mocked filesystem to the specified stream
void mfs_array_print(mocked_fs_s *array,  FILE *stream);

// Applies the data op to the mfs
// Returns EXIT_FAILURE if the op is out of MFS bounds, EXIT_SUCCESS otherwise
int mfs_apply_op(mocked_fs_s *mfs, data_op_s *op);

#endif //THESIS_CSEC_MOCKED_FS_H
