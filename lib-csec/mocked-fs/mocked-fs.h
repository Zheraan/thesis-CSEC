//
// Created by zheraan on 22/04/23.
//

#ifndef THESIS_CSEC_MOCKED_FS_H
#define THESIS_CSEC_MOCKED_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include "mfs-datatypes.h"
#include "../overseer.h"

// Prints the contents of the mocked filesystem to the specified stream
void mfs_array_print(mocked_fs_s *mfs, FILE *stream);

// Applies the data op to the mfs
// Returns EXIT_FAILURE if the op is out of MFS bounds, EXIT_SUCCESS otherwise
int mfs_apply_op(mocked_fs_s *mfs, const data_op_s *op);

#endif //THESIS_CSEC_MOCKED_FS_H
