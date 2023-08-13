//
// Created by zheraan on 05/05/23.
//

#include "timeout.h"

struct timeval timeout_gen(enum timeout_type type) {
    struct timeval ntv;

    uint32_t buf, modulo = 0, offset = 0, randomize = false;

    // Define the timeout range depending on the defined parameters
    switch (type) {
        case TIMEOUT_TYPE_P_HB:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_P_HB + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_P_HB + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_HS_HB:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_HS_HB + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_HS_HB + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_PROPOSITION:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_PROPOSITION + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_PROPOSITION + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_ACK:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_ACK + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_ACK + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_PROP_RETRANSMISSION:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_PROP_RETRANSMISSION + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_PROP_RETRANSMISSION + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_P_LIVENESS:
            ntv.tv_usec = MODULO(TIMEOUT_VALUE_P_LIVENESS + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(TIMEOUT_VALUE_P_LIVENESS + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            break;

        case TIMEOUT_TYPE_HS_ELECTION:
            if (TIMEOUT_RANGE_ELECTION > 0)
                modulo = TIMEOUT_RANGE_ELECTION + 1;
            offset = TIMEOUT_OFFSET_ELECTION;
            randomize = true;
            break;

        case TIMEOUT_TYPE_FUZZER:
            if (FUZZER_LATENCY_DISTRIBUTION_ENABLE == true) {
                evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
                double a = FUZZER_LATENCY_DISTRIBUTION_MINIMUM,
                        b = 100000000, // Maximum of the definition interval (minimum is 0)
                c = FUZZER_LATENCY_DISTRIBUTION_PROPORTION * 1000000;
                buf = MODULO(buf, (uint32_t) b);
                // Applying distribution function
                buf = a * (1 + (buf * (b - c) * (b - c)) / (c * (b - buf) * (b - buf)));
                if (buf > FUZZER_LATENCY_DISTRIBUTION_MAXIMUM)
                    buf = FUZZER_LATENCY_DISTRIBUTION_MAXIMUM;
                ntv.tv_usec = MODULO(buf, 1000000);
                ntv.tv_sec = DIVIDE(buf, 1000000);
            } else {
                if (TIMEOUT_RANGE_FUZZER > 0)
                    modulo = TIMEOUT_RANGE_FUZZER + 1;
                offset = TIMEOUT_OFFSET_FUZZER;
                randomize = true;
            }
            break;

        case TIMEOUT_TYPE_RANDOM_OPS:
            if (TIMEOUT_RANGE_RANDOM_OPS > 0)
                modulo = TIMEOUT_RANGE_RANDOM_OPS + 1;
            offset = TIMEOUT_OFFSET_RANDOM_OPS;
            randomize = true;
            break;

        case TIMEOUT_TYPE_KILL:
            if (TIMEOUT_RANGE_KILL > 0)
                modulo = TIMEOUT_RANGE_KILL + 1;
            offset = TIMEOUT_OFFSET_KILL;
            randomize = true;
            break;

        default:
            debug_log(0, stderr, "Unknown timeout type\n");
            errno = EUNKNOWN_TIMEOUT_TYPE;
            return ntv;
    }

    if (modulo > 0) {
        evutil_secure_rng_get_bytes(&buf, sizeof(uint32_t));
        buf = MODULO(buf, modulo); // Set the value inside the range
    } else {
        buf = 0;
    }

    if (randomize == true) {
        if (type != TIMEOUT_TYPE_FUZZER || FUZZER_LATENCY_DISTRIBUTION_ENABLE == false) {
            ntv.tv_usec = MODULO(buf + offset + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
            ntv.tv_sec = DIVIDE(buf + offset + TIMEOUT_GLOBAL_SLOWDOWN_OFFSET, 1000000);
        }
    }

    if (DEBUG_LEVEL >= 4)
        printf("[Created a timeout of type %d and a duration of %ld.%lds] ",
               type,
               ntv.tv_sec,
               ntv.tv_usec);
    return ntv;
}