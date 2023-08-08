//
// Created by zheraan on 22/04/23.
//

#include "data-op.h"

data_op_s *op_new() {
    data_op_s *nop = malloc(sizeof(data_op_s));
    if (nop == NULL) {
        perror("malloc new op");
        return NULL;
    }

    uint8_t buf;
    // Randomly pick a new character
    evutil_secure_rng_get_bytes(&buf, sizeof(uint8_t));
    buf %= 93; // Ensure value is no bigger than the printable character range
    nop->newval = buf + 33; // Ensure value is a printable character

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
            "row:       %d\n"
            "column:    %d\n"
            "newval:    %c\n",
            op->row,
            op->column,
            op->newval);
    return;
}