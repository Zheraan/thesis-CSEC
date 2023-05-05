//
// Created by zheraan on 22/04/23.
//

#include "mocked-fs.h"

void mfs_array_print(mocked_fs_s *array,  FILE *stream){
    fprintf(stream, "Local MFS array contents:\n");
    for (int i = 0; i < MOCKED_FS_ARRAY_ROWS; ++i) {
        for (int j = 0; j < MOCKED_FS_ARRAY_COLUMNS; ++j) {
            fprintf(stream, "%c ", array->array[i][j]);
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "\n");
    return;
}

int mfs_apply_op(mocked_fs_s *mfs, data_op_s *op){
    if (op->column >= MOCKED_FS_ARRAY_COLUMNS || op->row >= MOCKED_FS_ARRAY_ROWS){
        fprintf(stderr, "Data op out of MFS bounds\n");
        return EXIT_FAILURE;
    }
    mfs->array[op->row][op->column] = op->newval;
    mfs->nb_ops += 1;
    return EXIT_SUCCESS;
}