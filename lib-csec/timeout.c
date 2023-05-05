//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval timeout_gen(enum timeout_type type) {
    struct timeval ntv;

    int buf, modulo_sec = 0, modulo_usec = 0;

    // Define the timeout range depending on the defined parameters
    switch (type) {
        case TIMEOUT_TYPE_P_HB:
            if (TIMEOUT_RANGE_P_HB_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_P_HB_USEC + 1;
            if (TIMEOUT_RANGE_P_HB_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_P_HB_SEC + 1;
            break;

        case TIMEOUT_TYPE_HS_HB:
            if (TIMEOUT_RANGE_HS_HB_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_HS_HB_USEC + 1;
            if (TIMEOUT_RANGE_HS_HB_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_HS_HB_SEC + 1;
            break;

        case TIMEOUT_TYPE_PROPOSITION:
            ntv.tv_usec = TIMEOUT_VALUE_PROPOSITION_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_PROPOSITION_SEC;
            return ntv;

        case TIMEOUT_TYPE_ACK:
            ntv.tv_usec = TIMEOUT_VALUE_ACK_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_ACK_SEC;
            return ntv;

        case TIMEOUT_TYPE_ELECTION:
            if (TIMEOUT_RANGE_ELECTION_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_ELECTION_USEC + 1;
            if (TIMEOUT_RANGE_ELECTION_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_ELECTION_SEC + 1;
            break;

        case TIMEOUT_TYPE_FUZZER:
            if (TIMEOUT_RANGE_FUZZER_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_FUZZER_USEC + 1;
            if (TIMEOUT_RANGE_FUZZER_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_FUZZER_SEC + 1;
            break;

        case TIMEOUT_TYPE_RANDOM_OPS:
            if (TIMEOUT_RANGE_RANDOM_OPS_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_RANDOM_OPS_USEC + 1;
            if (TIMEOUT_RANGE_RANDOM_OPS_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_RANDOM_OPS_SEC + 1;
            break;

        default:
            fprintf(stderr, "Unknown timeout type\n");
            errno = EUNKOWN_TIMEOUT_TYPE;
            return ntv;
    }

    if (modulo_usec > 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(int));
        buf %= modulo_usec; // Set the value inside the range
        ntv.tv_usec = buf + TIMEOUT_OFFSET_USEC; // Add the offset
    } else {
        ntv.tv_usec = 0;
    }

    if (modulo_sec > 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(int));
        buf %= modulo_sec; // Set the value inside the range
        ntv.tv_sec = buf + TIMEOUT_OFFSET_SEC; // Add the offset
    } else {
        ntv.tv_sec = 0;
    }

    return ntv;
}