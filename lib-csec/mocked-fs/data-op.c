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

uint8_t op_compare(data_op_s *ref, data_op_s *val, FILE *stream) {
    uint8_t rv = CSEC_FLAG_DEFAULT;
    if (ref->newval != val->newval)
        rv |= OPCOMP_FLAG_INVALID_VALUE;
    if (ref->column != val->column)
        rv |= OPCOMP_FLAG_INVALID_COLUMN;
    if (ref->row != val->row)
        rv |= OPCOMP_FLAG_INVALID_ROW;

    if (rv == CSEC_FLAG_DEFAULT) {
        if (MONITORING_LEVEL >= 4) {
            fprintf(stream, "Op comparison result: OK.\n");
            if (INSTANT_FFLUSH) fflush(stream);
        }
        return rv;
    } else if (MONITORING_LEVEL >= 2) {
        fprintf(stream, "Op comparison result: \n");
        if ((rv & OPCOMP_FLAG_INVALID_ROW) == OPCOMP_FLAG_INVALID_ROW)
            fprintf(stream, "  - Invalid row: is '%d', should be '%d'\n", val->row, ref->row);
        if ((rv & OPCOMP_FLAG_INVALID_COLUMN) == OPCOMP_FLAG_INVALID_COLUMN)
            fprintf(stream, "  - Invalid column: is '%d', should be '%d'\n", val->column, ref->column);
        if ((rv & OPCOMP_FLAG_INVALID_VALUE) == OPCOMP_FLAG_INVALID_VALUE)
            fprintf(stream, "  - Invalid value: is '%c', should be '%c'\n", val->newval, ref->newval);
        if (INSTANT_FFLUSH) fflush(stream);
    }

    return rv;
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