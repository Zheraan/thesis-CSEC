//
// Created by zheraan on 22/04/23.
//

#include "data-op.h"

data_op_s *op_new(){
    data_op_s *nop = malloc(sizeof(data_op_s));
    if (nop == NULL) {
        perror("malloc new op");
        return NULL;
    }

    // Randomly pick a new character
    int buf;
    evutil_secure_rng_get_bytes(&buf, sizeof(int));
    buf %= 95; // Ensure value is no bigger than the alphanumeric range
    nop->newval = buf + 32; // Ensure value is within the alphanumeric range

    // Randomly pick a row
    evutil_secure_rng_get_bytes(&buf, sizeof(int));
    buf %= MOCKED_FS_ARRAY_ROWS;
    nop->row = buf;

    // Randomly pick a column
    evutil_secure_rng_get_bytes(&buf, sizeof(int));
    buf %= MOCKED_FS_ARRAY_COLUMNS;
    nop->column = buf;

    return nop;
}