//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval timeout_gen(enum timeout_type type) {
    struct timeval ntv;

    uint32_t buf, modulo_sec = 0, modulo_usec = 0, offset_sec = 0, offset_usec = 0, randomize = 1;

    // Define the timeout range depending on the defined parameters
    switch (type) {
        case TIMEOUT_TYPE_P_HB:
            ntv.tv_usec = TIMEOUT_VALUE_P_HB_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_P_HB_SEC;
            randomize = 0;
            break;

        case TIMEOUT_TYPE_HS_HB:
            ntv.tv_usec = TIMEOUT_VALUE_HS_HB_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_HS_HB_SEC;
            randomize = 0;
            break;

        case TIMEOUT_TYPE_PROPOSITION:
            ntv.tv_usec = TIMEOUT_VALUE_PROPOSITION_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_PROPOSITION_SEC;
            randomize = 0;
            break;

        case TIMEOUT_TYPE_ACK:
            ntv.tv_usec = TIMEOUT_VALUE_ACK_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_ACK_SEC;
            randomize = 0;
            break;

        case TIMEOUT_TYPE_PROP_RETRANSMISSION:
            ntv.tv_usec = TIMEOUT_VALUE_PROP_RETRANSMISSION_USEC;
            ntv.tv_sec = TIMEOUT_VALUE_PROP_RETRANSMISSION_SEC;
            randomize = 0;
            break;

        case TIMEOUT_TYPE_ELECTION:
            if (TIMEOUT_RANGE_ELECTION_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_ELECTION_USEC + 1;
            if (TIMEOUT_RANGE_ELECTION_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_ELECTION_SEC + 1;
            offset_sec = TIMEOUT_OFFSET_ELECTION_SEC;
            offset_usec = TIMEOUT_OFFSET_ELECTION_USEC;
            break;

        case TIMEOUT_TYPE_FUZZER:
            if (TIMEOUT_RANGE_FUZZER_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_FUZZER_USEC + 1;
            if (TIMEOUT_RANGE_FUZZER_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_FUZZER_SEC + 1;
            offset_sec = TIMEOUT_OFFSET_FUZZER_SEC;
            offset_usec = TIMEOUT_OFFSET_FUZZER_USEC;
            break;

        case TIMEOUT_TYPE_RANDOM_OPS:
            if (TIMEOUT_RANGE_RANDOM_OPS_USEC > 0)
                modulo_usec = TIMEOUT_RANGE_RANDOM_OPS_USEC + 1;
            if (TIMEOUT_RANGE_RANDOM_OPS_SEC > 0)
                modulo_sec = TIMEOUT_RANGE_RANDOM_OPS_SEC + 1;
            offset_sec = TIMEOUT_OFFSET_RANDOM_OPS_SEC;
            offset_usec = TIMEOUT_OFFSET_RANDOM_OPS_USEC;
            break;

        default:
            fprintf(stderr, "Unknown timeout type\n");
            fflush(stderr);
            errno = EUNKOWN_TIMEOUT_TYPE;
            return ntv;
    }

    if (randomize) {
        if (modulo_usec > 0) {
            evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
            buf %= modulo_usec; // Set the value inside the range
            ntv.tv_usec = buf + offset_usec; // Add the offset
        } else {
            ntv.tv_usec = 0 + offset_usec;
        }

        if (modulo_sec > 0) {
            evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
            buf %= modulo_sec; // Set the value inside the range
            ntv.tv_sec = buf + offset_sec; // Add the offset
        } else {
            ntv.tv_sec = 0 + offset_sec;
        }
    }

    if (DEBUG_LEVEL >= 4)
        printf("\n  - Created a timeout of type %d and a duration of %ldsec %ldusec\n",
               type,
               ntv.tv_sec,
               ntv.tv_usec);
    return ntv;
}