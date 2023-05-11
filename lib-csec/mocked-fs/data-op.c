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
    uint8_t buf;
    evutil_secure_rng_get_bytes(&buf, sizeof(uint8_t));
    buf %= 95; // Ensure value is no bigger than the alphanumeric range
    nop->newval = buf + 32; // Ensure value is within the alphanumeric range

    // Randomly pick a row
    evutil_secure_rng_get_bytes(&buf, sizeof(uint8_t));
    buf %= MOCKED_FS_ARRAY_ROWS;
    nop->row = buf;

    // Randomly pick a column
    evutil_secure_rng_get_bytes(&buf, sizeof(uint8_t));
    buf %= MOCKED_FS_ARRAY_COLUMNS;
    nop->column = buf;

    return nop;
}

void op_print(const data_op_s *op, FILE *stream) {
    fprintf(stream,
            "column:    %ld\n"
            "row:       %ld\n"
            "newval:    %d\n",
            op->column,
            op->row,
            op->newval);
    return;
}