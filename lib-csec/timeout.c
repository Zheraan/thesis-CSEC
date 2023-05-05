//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval timeout_random(enum timeout_type type) {
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
            if (TIMEOUT_RANGE_PROPOSITION_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_PROPOSITION_USEC + 1;
            if (TIMEOUT_RANGE_PROPOSITION_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_PROPOSITION_SEC + 1;
            break;

        case TIMEOUT_TYPE_ACK:
            if (TIMEOUT_RANGE_ACK_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_ACK_USEC + 1;
            if (TIMEOUT_RANGE_ACK_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_ACK_SEC + 1;
            break;

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

        default:
            fprintf(stderr, "Unknown timeout type\n");
            errno = EUNKOWN_TIMEOUT_TYPE;
            return ntv;
    }

    if (modulo_usec != 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(int));
        buf %= modulo_usec; // Ensure value is no bigger than the alphanumeric range
        ntv.tv_usec = buf + TIMEOUT_OFFSET; // Ensure value is within the alphanumeric range
    } else {
        ntv.tv_usec = 0;
    }

    if (modulo_sec != 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(int));
        buf %= modulo_sec; // Ensure value is no bigger than the alphanumeric range
        ntv.tv_sec = buf + TIMEOUT_OFFSET; // Ensure value is within the alphanumeric range
    } else {
        ntv.tv_sec = 0;
    }

    return ntv;
}